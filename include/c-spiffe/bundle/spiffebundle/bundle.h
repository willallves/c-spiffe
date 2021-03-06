#ifndef INCLUDE_BUNDLE_SPIFFEBUNDLE_BUNDLE_H
#define INCLUDE_BUNDLE_SPIFFEBUNDLE_BUNDLE_H

#include "c-spiffe/bundle/jwtbundle/bundle.h"
#include "c-spiffe/bundle/x509bundle/bundle.h"
#include "c-spiffe/spiffeid/trustdomain.h"
#include "c-spiffe/utils/util.h"
#include <openssl/x509.h>
#include <threads.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // bundle trust domain
    spiffeid_TrustDomain td;
    // read write mutex
    mtx_t mtx;
    // time duration (...)
    struct timespec refresh_hint;
    // sequence number
    int64_t seq_number;
    // hash of jwt authorities
    map_string_EVP_PKEY *jwt_auths;
    // STB array of x509 certificates
    X509 **x509_auths;
} spiffebundle_Bundle;

spiffebundle_Bundle *spiffebundle_New(const spiffeid_TrustDomain td);
void spiffebundle_Bundle_Free(spiffebundle_Bundle* bundle);

spiffebundle_Bundle *spiffebundle_Load(const spiffeid_TrustDomain td,
                                       const char *path, err_t *err);
spiffebundle_Bundle *spiffebundle_Parse(const spiffeid_TrustDomain td,
                                        const char *bundle_bytes, err_t *err);
spiffebundle_Bundle *spiffebundle_FromX509Bundle(x509bundle_Bundle *bundle);
spiffebundle_Bundle *spiffebundle_FromJWTBundle(jwtbundle_Bundle *bundle);
spiffebundle_Bundle *
spiffebundle_FromX509Authorities(const spiffeid_TrustDomain td, X509 **auths);
spiffebundle_Bundle *
spiffebundle_FromJWTAuthorities(const spiffeid_TrustDomain td,
                                map_string_EVP_PKEY *auths);
spiffeid_TrustDomain
spiffebundle_Bundle_TrustDomain(const spiffebundle_Bundle *b);
X509 **spiffebundle_Bundle_X509Authorities(spiffebundle_Bundle *b);
void spiffebundle_Bundle_AddX509Authority(spiffebundle_Bundle *b, X509 *auth);
void spiffebundle_Bundle_RemoveX509Authority(spiffebundle_Bundle *b,
                                             const X509 *auth);
bool spiffebundle_Bundle_HasX509Authority(spiffebundle_Bundle *b,
                                          const X509 *auth);
void spiffebundle_Bundle_SetX509Authorities(spiffebundle_Bundle *b,
                                            X509 **auths);
map_string_EVP_PKEY *
spiffebundle_Bundle_JWTAuthorities(spiffebundle_Bundle *b);
EVP_PKEY *spiffebundle_Bundle_FindJWTAuthority(spiffebundle_Bundle *b,
                                               const char *keyID, bool *suc);
bool spiffebundle_Bundle_HasJWTAuthority(spiffebundle_Bundle *b,
                                         const char *keyID);
err_t spiffebundle_Bundle_AddJWTAuthority(spiffebundle_Bundle *b,
                                          const char *keyID, EVP_PKEY *auth);
void spiffebundle_Bundle_RemoveJWTAuthority(spiffebundle_Bundle *b,
                                            const char *keyID);
void spiffebundle_Bundle_SetJWTAuthorities(spiffebundle_Bundle *b,
                                           map_string_EVP_PKEY *auths);
bool spiffebundle_Bundle_Empty(spiffebundle_Bundle *b);
struct timespec spiffebundle_Bundle_RefreshHint(spiffebundle_Bundle *b,
                                                bool *suc);
void spiffebundle_Bundle_SetRefreshHint(spiffebundle_Bundle *b,
                                        const struct timespec *refHint);
void spiffebundle_Bundle_ClearRefreshHint(spiffebundle_Bundle *b);
int64_t spiffebundle_Bundle_SequenceNumber(spiffebundle_Bundle *b, bool *suc);
void spiffebundle_Bundle_SetSequenceNumber(spiffebundle_Bundle *b,
                                           int64_t seq_number);
void spiffebundle_Bundle_ClearSequenceNumber(spiffebundle_Bundle *b);
string_t spiffebundle_Bundle_Marshal(spiffebundle_Bundle *b, err_t *err);
spiffebundle_Bundle *spiffebundle_Bundle_Clone(spiffebundle_Bundle *b);
x509bundle_Bundle *spiffebundle_Bundle_X509Bundle(spiffebundle_Bundle *b);
jwtbundle_Bundle *spiffebundle_Bundle_JWTBundle(spiffebundle_Bundle *b);
spiffebundle_Bundle *spiffebundle_Bundle_GetBundleForTrustDomain(
    spiffebundle_Bundle *b, const spiffeid_TrustDomain td, err_t *err);
x509bundle_Bundle *spiffebundle_Bundle_GetX509BundleForTrustDomain(
    spiffebundle_Bundle *b, const spiffeid_TrustDomain td, err_t *err);
jwtbundle_Bundle *spiffebundle_Bundle_GetJWTBundleForTrustDomain(
    spiffebundle_Bundle *b, const spiffeid_TrustDomain td, err_t *err);
bool spiffebundle_Bundle_Equal(const spiffebundle_Bundle *b1,
                               const spiffebundle_Bundle *b2);
bool spiffebundle_refreshHintEqual(const struct timespec *t1,
                                   const struct timespec *t2);
bool spiffebundle_sequenceNumberEqual(const int64_t a, const int64_t b);
struct timespec spiffebundle_copyRefreshHint(const struct timespec *ts);
void spiffebundle_Bundle_Free(spiffebundle_Bundle *b);

#ifdef __cplusplus
}
#endif

#endif
