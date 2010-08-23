#ifndef H__dfsch__lib_crypto__
#define H__dfsch__lib_crypto__

#include <dfsch/dfsch.h>
#include <stdint.h>

typedef struct dfsch_block_cipher_context_t dfsch_block_cipher_context_t;

typedef void (*dfsch_block_cipher_operation_t)
(dfsch_block_cipher_context_t* context,
 void* in,
 void* out);
typedef void (*dfsch_block_cipher_setup_t)
(dfsch_block_cipher_context_t* context,
 uint8_t* key,
 size_t key_len);

typedef struct dfsch_block_cipher_t {
  dfsch_type_t type;
  char* name;
  
  size_t block_size;
  
  dfsch_block_cipher_operation_t encrypt;
  dfsch_block_cipher_operation_t decrypt;
  dfsch_block_cipher_setup_t setup;

  DFSCH_ALIGN8_DUMMY
} DFSCH_ALIGN8_ATTR dfsch_block_cipher_t;

dfsch_block_cipher_t* dfsch_block_cipher(dfsch_object_t* obj);
#define DFSCH_BLOCK_CIPHER_ARG(al, name)                             \
  DFSCH_GENERIC_ARG(al, name, dfsch_block_cipher_t*, dfsch_block_cipher)

extern dfsch_block_cipher_t dfsch_crypto_aes_cipher;
#define DFSCH_CRYPTO_AES_CIPHER (&dfsch_crypto_aes_cipher)
extern dfsch_block_cipher_t dfsch_crypto_xtea_cipher;
#define DFSCH_CRYPTO_XTEA_CIPHER (&dfsch_crypto_xtea_cipher)
extern dfsch_block_cipher_t dfsch_crypto_blowfish_cipher;
#define DFSCH_CRYPTO_BLOWFISH_CIPHER (&dfsch_crypto_blowfish_cipher)

struct dfsch_block_cipher_context_t{
  dfsch_block_cipher_t* cipher;
};

extern dfsch_type_t dfsch_block_cipher_type;
#define DFSCH_BLOCK_CIPHER_TYPE (&dfsch_block_cipher_type)

dfsch_block_cipher_context_t* 
dfsch_setup_block_cipher(dfsch_block_cipher_t* cipher,
                         uint8_t* key,
                         size_t key_len);
int dfsch_block_cipher_context_p(dfsch_object_t* obj);
dfsch_block_cipher_context_t*  dfsch_block_cipher_context(dfsch_object_t* obj);


#define DFSCH_BLOCK_CIPHER_CONTEXT_ARG(al, name)             \
  DFSCH_GENERIC_ARG(al, name, dfsch_block_cipher_context_t*, \
                    dfsch_block_cipher_context)


typedef struct dfsch_block_cipher_mode_context_t 
dfsch_block_cipher_mode_context_t;

typedef void (*dfsch_block_cipher_mode_operation_t)
(dfsch_block_cipher_mode_context_t* context,
 uint8_t* in,
 uint8_t* out,
 size_t blocks);

typedef void (*dfsch_block_cipher_mode_setup_t)
(dfsch_block_cipher_mode_context_t* cipher,
 uint8_t* iv,
 size_t iv_len);

typedef struct dfsch_block_cipher_mode_t {
  dfsch_type_t type;

  char* name;

  dfsch_block_cipher_mode_operation_t decrypt;
  dfsch_block_cipher_mode_operation_t encrypt;
  dfsch_block_cipher_mode_setup_t setup;
  DFSCH_ALIGN8_DUMMY
} DFSCH_ALIGN8_ATTR dfsch_block_cipher_mode_t;

extern dfsch_block_cipher_mode_t dfsch_crypto_ecb_mode;
#define DFSCH_CRYPTO_ECB_MODE (&dfsch_crypto_ecb_mode)
extern dfsch_block_cipher_mode_t dfsch_crypto_cbc_mode;
#define DFSCH_CRYPTO_CBC_MODE (&dfsch_crypto_cbc_mode)
extern dfsch_block_cipher_mode_t dfsch_crypto_cfb_mode;
#define DFSCH_CRYPTO_CFB_MODE (&dfsch_crypto_cfb_mode)
extern dfsch_block_cipher_mode_t dfsch_crypto_ofb_mode;
#define DFSCH_CRYPTO_OFB_MODE (&dfsch_crypto_ofb_mode)
extern dfsch_block_cipher_mode_t dfsch_crypto_ctr_mode;
#define DFSCH_CRYPTO_CTR_MODE (&dfsch_crypto_ctr_mode)

dfsch_block_cipher_t* dfsch_block_cipher_mode(dfsch_object_t* obj);
#define DFSCH_BLOCK_CIPHER_MODE_ARG(al, name)                           \
  DFSCH_GENERIC_ARG(al, name, dfsch_block_cipher_mode_t*,               \
                    dfsch_block_cipher_mode)

extern dfsch_type_t dfsch_block_cipher_mode_type;
#define DFSCH_BLOCK_CIPHER_MODE_TYPE (&dfsch_block_cipher_mode_type)


dfsch_block_cipher_mode_context_t* 
dfsch_setup_block_cipher_mode(dfsch_block_cipher_mode_t* mode,
                              dfsch_block_cipher_context_t* cipher,
                              uint8_t* iv,
                              size_t iv_len);
int dfsch_block_cipher_mode_context_p(dfsch_object_t* obj);
dfsch_block_cipher_mode_context_t*  
dfsch_block_cipher_mode_context(dfsch_object_t* obj);


#define DFSCH_BLOCK_CIPHER_MODE_CONTEXT_ARG(al, name)             \
  DFSCH_GENERIC_ARG(al, name, dfsch_block_cipher_mode_context_t*, \
                    dfsch_block_cipher_mode_context)


struct dfsch_block_cipher_mode_context_t {
  dfsch_block_cipher_mode_t* mode;
  dfsch_block_cipher_context_t* cipher;
};


typedef struct dfsch_crypto_hash_context_t dfsch_crypto_hash_context_t;

typedef void (*dfsch_crypto_hash_setup_t)(dfsch_crypto_hash_context_t* ctx,
                                          uint8_t* key,
                                          size_t key_len);
typedef void (*dfsch_crypto_hash_process_t)(dfsch_crypto_hash_context_t* ctx,
                                            uint8_t* buf,
                                            size_t len);
typedef void (*dfsch_crypto_hash_result_t)(dfsch_crypto_hash_context_t* ctx,
                                           uint8_t* res);

typedef struct dfsch_crypto_hash_t {
  dfsch_type_t type;
  
  char* name;

  size_t block_len; /* size of block used internally by hash function */
  size_t result_len;
  
  dfsch_crypto_hash_setup_t setup;
  dfsch_crypto_hash_process_t process;
  dfsch_crypto_hash_result_t result;
} dfsch_crypto_hash_t;

extern dfsch_type_t dfsch_crypto_hash_type;
#define DFSCH_CRYPTO_HASH_TYPE (&dfsch_crypto_hash_type)

dfsch_block_cipher_t* dfsch_crypto_hash(dfsch_object_t* obj);
#define DFSCH_CRYPTO_HASH_ARG(al, name)                                 \
  DFSCH_GENERIC_ARG(al, name, dfsch_crypto_hash_t*,                     \
                    dfsch_crypto_hash)

dfsch_crypto_hash_context_t* dfsch_crypto_hash_setup(dfsch_crypto_hash_t* hash,
                                                     uint8_t* key,
                                                     size_t keylen);
int dfsch_crypto_hash_context_p(dfsch_object_t* obj);
dfsch_crypto_hash_context_t* dfsch_crypto_hash_context(dfsch_object_t* obj);

#define DFSCH_CRYPTO_HASH_CONTEXT_ARG(al, name)             \
  DFSCH_GENERIC_ARG(al, name, dfsch_crypto_hash_context_t*, \
                    dfsch_crypto_hash_context)

struct dfsch_crypto_hash_context_t {
  dfsch_crypto_hash_t* algo;
};

extern dfsch_crypto_hash_t dfsch_crypto_sha256;
#define DFSCH_CRYPTO_SHA256 (&dfsch_crypto_sha256)
extern dfsch_crypto_hash_t dfsch_crypto_sha1;
#define DFSCH_CRYPTO_SHA1 (&dfsch_crypto_sha1)
extern dfsch_crypto_hash_t dfsch_crypto_md5;
#define DFSCH_CRYPTO_MD5 (&dfsch_crypto_md5)
extern dfsch_crypto_hash_t dfsch_crypto_md4;
#define DFSCH_CRYPTO_MD4 (&dfsch_crypto_md4)


dfsch_crypto_hash_t* dfsch_crypto_make_hmac(dfsch_crypto_hash_t* hash);
extern dfsch_type_t dfsch_crypto_hmac_type;
#define DFSCH_CRYPTO_HMAC_TYPE (&dfsch_crypto_hmac_type)


void dfsch_crypto_curve25519(uint8_t *mypublic, 
                             const uint8_t *secret, 
                             const uint8_t *basepoint);


#endif