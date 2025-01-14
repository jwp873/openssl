
/*
 * Copyright 2019 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <openssl/aes.h>

typedef struct prov_gcm_hw_st PROV_GCM_HW;

#define GCM_IV_DEFAULT_SIZE 12/* IV's for AES_GCM should normally be 12 bytes */
#define GCM_IV_MAX_SIZE     64
#define GCM_TAG_MAX_SIZE    16


#if defined(OPENSSL_CPUID_OBJ) && defined(__s390__)
/*-
 * KMA-GCM-AES parameter block - begin
 * (see z/Architecture Principles of Operation >= SA22-7832-11)
 */
typedef struct S390X_kma_params_st {
    unsigned char reserved[12];
    union {
        unsigned int w;
        unsigned char b[4];
    } cv; /* 32 bit counter value */
    union {
        unsigned long long g[2];
        unsigned char b[16];
    } t; /* tag */
    unsigned char h[16]; /* hash subkey */
    unsigned long long taadl; /* total AAD length */
    unsigned long long tpcl; /* total plaintxt/ciphertxt len */
    union {
        unsigned long long g[2];
        unsigned int w[4];
    } j0;                   /* initial counter value */
    unsigned char k[32];    /* key */
} S390X_KMA_PARAMS;

#endif

typedef struct prov_gcm_ctx_st {
    int enc;                /* Set to 1 if we are encrypting or 0 otherwise */
    int mode;               /* The mode that we are using */
    size_t keylen;
    int ivlen;
    size_t ivlen_min;
    int taglen;
    int key_set;            /* Set if key initialised */
    int iv_state;           /* set to one of IV_STATE_XXX */
    int iv_gen_rand;        /* No IV was specified, so generate a rand IV */
    int iv_gen;             /* It is OK to generate IVs */
    int tls_aad_pad_sz;
    int tls_aad_len;        /* TLS AAD length */
    uint64_t tls_enc_records;   /* Number of TLS records encrypted */

    /*
     * num contains the number of bytes of |iv| which are valid for modes that
     * manage partial blocks themselves.
     */
    size_t num;
    size_t bufsz;           /* Number of bytes in buf */
    uint64_t flags;

    unsigned int pad : 1;   /* Whether padding should be used or not */

    unsigned char iv[GCM_IV_MAX_SIZE]; /* Buffer to use for IV's */
    unsigned char buf[AES_BLOCK_SIZE];     /* Buffer of partial blocks processed via update calls */

    OPENSSL_CTX *libctx;    /* needed for rand calls */
    const PROV_GCM_HW *hw;  /* hardware specific methods */
    GCM128_CONTEXT gcm;
    ctr128_f ctr;
    const void *ks;
} PROV_GCM_CTX;

typedef struct prov_aes_gcm_ctx_st {
    PROV_GCM_CTX base;          /* must be first entry in struct */
    union {
        OSSL_UNION_ALIGN;
        AES_KEY ks;
    } ks;                       /* AES key schedule to use */

    /* Platform specific data */
    union {
        int dummy;
#if defined(OPENSSL_CPUID_OBJ) && defined(__s390__)
        struct {
            union {
                OSSL_UNION_ALIGN;
                S390X_KMA_PARAMS kma;
            } param;
            unsigned int fc;
            unsigned char ares[16];
            unsigned char mres[16];
            unsigned char kres[16];
            int areslen;
            int mreslen;
            int kreslen;
            int res;
        } s390x;
#endif /* defined(OPENSSL_CPUID_OBJ) && defined(__s390__) */
    } plat;
} PROV_AES_GCM_CTX;

PROV_CIPHER_FUNC(int, GCM_setkey, (PROV_GCM_CTX *ctx, const unsigned char *key,
                                   size_t keylen));
PROV_CIPHER_FUNC(int, GCM_setiv, (PROV_GCM_CTX *dat, const unsigned char *iv,
                                  size_t ivlen));
PROV_CIPHER_FUNC(int, GCM_aadupdate, (PROV_GCM_CTX *ctx,
                                      const unsigned char *aad, size_t aadlen));
PROV_CIPHER_FUNC(int, GCM_cipherupdate, (PROV_GCM_CTX *ctx,
                                         const unsigned char *in, size_t len,
                                         unsigned char *out));
PROV_CIPHER_FUNC(int, GCM_cipherfinal, (PROV_GCM_CTX *ctx, unsigned char *tag));
PROV_CIPHER_FUNC(int, GCM_oneshot, (PROV_GCM_CTX *ctx, unsigned char *aad,
                                    size_t aad_len, const unsigned char *in,
                                    size_t in_len, unsigned char *out,
                                    unsigned char *tag, size_t taglen));
struct prov_gcm_hw_st {
  OSSL_GCM_setkey_fn setkey;
  OSSL_GCM_setiv_fn setiv;
  OSSL_GCM_aadupdate_fn aadupdate;
  OSSL_GCM_cipherupdate_fn cipherupdate;
  OSSL_GCM_cipherfinal_fn cipherfinal;
  OSSL_GCM_oneshot_fn oneshot;
};
const PROV_GCM_HW *PROV_AES_HW_gcm(size_t keybits);

#if !defined(OPENSSL_NO_ARIA) && !defined(FIPS_MODE)

#include "internal/aria.h"

typedef struct prov_aria_gcm_ctx_st {
    PROV_GCM_CTX base;              /* must be first entry in struct */
    union {
        OSSL_UNION_ALIGN;
        ARIA_KEY ks;
    } ks;
} PROV_ARIA_GCM_CTX;
const PROV_GCM_HW *PROV_ARIA_HW_gcm(size_t keybits);

#endif /* !defined(OPENSSL_NO_ARIA) && !defined(FIPS_MODE) */
