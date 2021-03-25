#include "verify.h"
#include "../../../internal/x509util/src/util.h"
#include <openssl/pem.h>

static void X509_STORE_add_stb_array(X509_STORE *store, X509 **x509_stb_array)
{
    if(store) {
        for(size_t i = 0, size = arrlenu(x509_stb_array); i < size; ++i) {
            if(x509_stb_array[i]) {
                X509_STORE_add_cert(store, x509_stb_array[i]);
            }
        }
    }
}

// verify chain
static X509 ***verifyX509(X509 *leaf, X509 **roots, X509 **inters, err_t *err)
{
    X509_STORE *store = X509_STORE_new();
    // adding intermediate certificates to the store
    X509_STORE_add_stb_array(store, inters);
    // adding root certificates to the store
    X509_STORE_add_stb_array(store, roots);

    X509_STORE_CTX *store_ctx = X509_STORE_CTX_new();
    // setup the store context for verification
    X509_STORE_CTX_init(store_ctx, store, leaf, NULL);

    const int ret = X509_verify_cert(store_ctx);
    X509 ***chains = NULL;

    if(ret > 0) {
        // a complete chain could be built
        STACK_OF(X509) *certs_stack = X509_STORE_CTX_get1_chain(store_ctx);

        X509 **certs_chain = NULL;
        for(int i = 0, size = sk_X509_num(certs_stack); i < size; ++i) {
            // from peer to root
            X509 *cert = sk_X509_value(certs_stack, i);
            /// TODO: check if it is necessary to up the ref count
            X509_up_ref(cert);
            arrput(certs_chain, cert);
        }

        sk_X509_pop_free(certs_stack, X509_free);

        X509 ***chains = NULL;
        arrput(chains, certs_chain);

        *err = NO_ERROR;
    } else if(ret == 0) {
        // could not build complete chain or no chain at all
        *err = ERROR1;
    } else {
        // X509_verify_cert was invoked incorrectly
        /// TODO: handle error
        *err = ERROR2;
    }

    X509_STORE_free(store);
    X509_STORE_CTX_free(store_ctx);

    return chains;
}

X509 ***x509svid_ParseAndVerify(byte **raw_certs, x509bundle_Source *source,
                                spiffeid_ID *id, err_t *err)
{
    X509 **certs = NULL;

    for(size_t i = 0, size = arrlenu(raw_certs); i < size; ++i) {
        const byte *tmp = raw_certs[i];
        X509 *cert = d2i_X509(NULL, &tmp, arrlen(raw_certs[i]));
        if(cert) {
            arrput(certs, cert);
        } else {
            // unable to parse certificate
            *err = ERROR1;
            // free them all
            for(size_t j = 0, size2 = arrlenu(certs); j < size2; ++j) {
                X509_free(certs[j]);
            }
            return NULL;
        }
    }

    return x509svid_Verify(certs, source, id, err);
}

X509 ***x509svid_Verify(X509 **certs, x509bundle_Source *source,
                        spiffeid_ID *id, err_t *err)
{
    // set id to NULL
    memset(id, 0, sizeof *id);

    if(arrlenu(certs) > 0 && source) {
        X509 *leaf = certs[0];
        spiffeid_ID leaf_id = x509svid_IDFromCert(leaf, err);

        if(!(*err)) {
            const uint32_t usage = X509_get_key_usage(leaf);

            if(!(usage & KU_KEY_CERT_SIGN) && !(usage & KU_CRL_SIGN)
               && !X509_check_ca(leaf)) {
                x509bundle_Bundle *bundle
                    = x509bundle_Source_GetX509BundleForTrustDomain(
                        source, spiffeid_ID_TrustDomain(leaf_id), err);

                if(!(*err)) {
                    // root certificates
                    X509 **x509_auths
                        = x509bundle_Bundle_X509Authorities(bundle);
                    // only intermediate certificates remaining
                    arrdel(certs, 0);
                    // verify chains
                    X509 ***chains = verifyX509(leaf, x509_auths, certs, err);

                    // insert leaf back up
                    arrins(certs, 0, leaf);
                    // freeing copied certificates
                    for(size_t i = 0, size = arrlenu(x509_auths); i < size;
                        ++i) {
                        X509_free(x509_auths[i]);
                    }
                    arrfree(x509_auths);

                    if(!(*err)) {
                        *id = leaf_id;
                        return chains;
                    } else {
                        // could not verify leaf certificate
                        *err = ERROR6;
                    }
                } else {
                    // could not get X509 bundle
                    *err = ERROR5;
                }
            } else {
                // wrong key usage or leaf is CA
                *err = ERROR4;
            }
        } else {
            // could not get leaf spiffe id
            *err = ERROR3;
        }
    } else if(arrlenu(certs) == 0) {
        // empty certificates chain
        *err = ERROR1;
    } else {
        // bundle is NULL
        *err = ERROR2;
    }

    return NULL;
}

spiffeid_ID x509svid_IDFromCert(X509 *cert, err_t *err)
{
    spiffeid_ID id = { NULL, NULL };
    if(cert) {
        const int nid = NID_subject_alt_name;
        STACK_OF(GENERAL_NAME) *san_names
            = (STACK_OF(GENERAL_NAME) *) X509_get_ext_d2i(cert, nid, NULL,
                                                          NULL);
        int san_name_num = sk_GENERAL_NAME_num(san_names);

        if(san_name_num == 1) {
            const GENERAL_NAME *name = sk_GENERAL_NAME_value(san_names, 0);
            string_t uri_name
                = string_new((char *) name->d.uniformResourceIdentifier->data);

            id = spiffeid_FromString(uri_name, err);
        } else if(san_name_num == 0) {
            // certificate contains no URI SAN
            *err = ERROR1;
        } else {
            // certificate contains more than one URI SAN
            *err = ERROR2;
        }

        sk_GENERAL_NAME_pop_free(san_names, GENERAL_NAME_free);
    } else
        // null certificate
        *err = ERROR3;

    return id;
}