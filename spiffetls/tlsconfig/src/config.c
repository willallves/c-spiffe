#include "spiffetls/tlsconfig/src/config.h"
#include "spiffetls/tlsconfig/src/authorizer.h"
#include "svid/x509svid/src/verify.h"

// tlsconfig_Trace *__trace;

void tlsconfig_Option_apply(tlsconfig_Option *op, tlsconfig_options *options)
{
    if(op->type == TLSCONFIG_FUNC) {
        op->source.func(options);
    }
}

tlsconfig_Option *tlsconfig_OptionFromFunc(tlsconfig_option fn)
{
    tlsconfig_Option *option = malloc(sizeof *option);
    option->type = TLSCONFIG_FUNC;
    option->source.func = fn;

    return option;
}

tlsconfig_options *tlsconfig_newOptions(tlsconfig_Option **opts)
{
    tlsconfig_options *out = malloc(sizeof *out);
    memset(out, 0, sizeof *out);

    for(size_t i = 0, size = arrlenu(opts); i < size; ++i) {
        tlsconfig_Option_apply(opts[i], out);
    }
    return out;
}

// static void setTrace(tlsconfig_options *opts) { opts->trace = __trace; }

// tlsconfig_Option *tlsconfig_WithTrace(tlsconfig_Trace *trace)
// {
//     __trace = trace;
//     return tlsconfig_OptionFromFunc(setTrace);
// }

SSL_CTX *tlsconfig_TLSClientConfig(x509bundle_Source *bundle,
                                   tlsconfig_Authorizer *authorizer,
                                   tlsconfig_Option **opts)
{
    // dummy
    return NULL;
}

struct hookTLSClientConfig_st {
    x509bundle_Source *bundle;
    tlsconfig_Authorizer *authorizer;
    tlsconfig_Option **opts;
};

/// TODO: change function name
static int hookTLSClientConfig_cb(X509_STORE_CTX *store_ctx, void *arg)
{
    struct hookTLSClientConfig_st *config = arg;
    spiffeid_ID id;
    bool suc = x509svid_Verify_cb(store_ctx, config->bundle, &id);

    if(suc) {
        return tlsconfig_ApplyAuthorizer(config->authorizer, id,
                                         /*unused arg*/ NULL)
                       == MATCH_OK
                   ? 1
                   : 0;
    } else {
        return 0;
    }
}

bool tlsconfig_HookTLSClientConfig(SSL_CTX *ctx, x509bundle_Source *bundle,
                                   tlsconfig_Authorizer *authorizer,
                                   tlsconfig_Option **opts)
{
    tlsconfig_resetAuthFields(ctx);
    // dummy call
    SSL_CTX_set_ecdh_auto(ctx, 1);
    /// WARNING: temporary solution

    struct hookTLSClientConfig_st *safe_arg = malloc(sizeof *safe_arg);
    safe_arg->bundle = bundle;
    safe_arg->authorizer = authorizer;
    safe_arg->opts = opts;

    SSL_CTX_set_cert_verify_callback(ctx, hookTLSClientConfig_cb, safe_arg);

    return true;
}

struct hookMTLSClientConfig_st {
    x509svid_Source *svid;
    x509bundle_Source *bundle;
    tlsconfig_Authorizer *authorizer;
    tlsconfig_Option **opts;
};

struct hookMTLSClientConfig_st *__config = NULL;
/// TODO: change function name
static int hookMTLSClientConfig_cb(SSL *ssl, X509 **cert, EVP_PKEY **pkey)
{
    err_t err;
    x509svid_SVID *svid = x509svid_Source_GetX509SVID(__config->svid, &err);

    if(!err && svid) {
        /// TODO: check if it is needed to up the reference
        *cert = svid->certs[0];
        /// TODO: check if it is needed to up the reference
        *pkey = svid->private_key;

        return 1;
    }

    return 0;
}

bool tlsconfig_HookMTLSClientConfig(SSL_CTX *ctx, x509svid_Source *svid,
                                    x509bundle_Source *bundle,
                                    tlsconfig_Authorizer *authorizer,
                                    tlsconfig_Option **opts)
{
    tlsconfig_resetAuthFields(ctx);
    // dummy call
    SSL_CTX_set_ecdh_auto(ctx, 1);

    err_t err;
    x509svid_SVID *my_svid = x509svid_Source_GetX509SVID(svid, &err);

    if(!err && svid) {
        if(arrlenu(my_svid->certs) > 0 && my_svid->private_key) {
            const int ret1 = SSL_CTX_use_certificate(ctx, my_svid->certs[0]);
            const int ret2 = SSL_CTX_use_PrivateKey(ctx, my_svid->private_key);

            if(ret1 <= 0 || ret2 <= 0) {
                return false;
            }
        } else {
            /// TODO: handle error
        }
    } else {
        /// TODO: handle error
    }

    struct hookTLSClientConfig_st *safe_arg1 = malloc(sizeof *safe_arg1);
    safe_arg1->bundle = bundle;
    safe_arg1->authorizer = authorizer;
    safe_arg1->opts = opts;
    SSL_CTX_set_cert_verify_callback(ctx, hookTLSClientConfig_cb, safe_arg1);

    return true;
}

// void tlsconfig_HookMTLSWebClientConfig(SSL_CTX *ctx, x509svid_Source *svid,
//                                        x509util_CertPool *roots,
//                                        tlsconfig_Option **opts)
// {}

void tlsconfig_resetAuthFields(SSL_CTX *ctx)
{
    // config.Certificates = nil - ok
    // config.ClientAuth = tls.NoClientCert
    // config.GetCertificate = nil - ok
    // config.GetClientCertificate = nil - ok
    // config.InsecureSkipVerify = false - ok
    // config.NameToCertificate = nil //nolint:staticcheck // setting to nil is
    // OK config.RootCAs = nil

    // clear certificates chain
    SSL_CTX_clear_chain_certs(ctx);
    // no callback function set
    SSL_CTX_set_client_hello_cb(ctx, NULL, NULL);
    // no callback function set
    SSL_CTX_set_client_cert_cb(ctx, NULL);
    // set default verification
    SSL_CTX_set_cert_verify_callback(ctx, NULL, NULL);
}

void tlsconfig_Option_Free(tlsconfig_Option *option)
{
    if(option) {
        free(option);
    }
}