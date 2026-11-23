// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

// Back side: AWS-LC's SHA-2. Implementations are written out per algorithm so
// each AWS-LC type, constant, and function binding remains visible to review.

#include <openssl/sha.h>

#include "internal/backend/digests.h"

// SHA-224

size_t awslc_prov_sha224_ctx_size(void) { return sizeof(SHA256_CTX); }

size_t awslc_prov_sha224_digest_size(void) { return SHA224_DIGEST_LENGTH; }

size_t awslc_prov_sha224_block_size(void) { return SHA224_CBLOCK; }

int awslc_prov_sha224_init(void *ctx) {
  if (ctx == NULL) {
    return 0;
  }
  return SHA224_Init((SHA256_CTX *)ctx);
}

int awslc_prov_sha224_update(void *ctx, const void *data, size_t len) {
  if (ctx == NULL || (data == NULL && len != 0)) {
    return 0;
  }
  return SHA224_Update((SHA256_CTX *)ctx, data, len);
}

int awslc_prov_sha224_final(void *ctx, unsigned char *out, size_t out_size) {
  if (ctx == NULL || out == NULL || out_size < SHA224_DIGEST_LENGTH) {
    return 0;
  }
  return SHA224_Final(out, (SHA256_CTX *)ctx);
}

int awslc_prov_sha224_copy(void *dst, const void *src) {
  if (dst == NULL || src == NULL) {
    return 0;
  }
  *(SHA256_CTX *)dst = *(const SHA256_CTX *)src;
  return 1;
}

// SHA-256

size_t awslc_prov_sha256_ctx_size(void) { return sizeof(SHA256_CTX); }

size_t awslc_prov_sha256_digest_size(void) { return SHA256_DIGEST_LENGTH; }

size_t awslc_prov_sha256_block_size(void) { return SHA256_CBLOCK; }

int awslc_prov_sha256_init(void *ctx) {
  if (ctx == NULL) {
    return 0;
  }
  return SHA256_Init((SHA256_CTX *)ctx);
}

int awslc_prov_sha256_update(void *ctx, const void *data, size_t len) {
  if (ctx == NULL || (data == NULL && len != 0)) {
    return 0;
  }
  return SHA256_Update((SHA256_CTX *)ctx, data, len);
}

int awslc_prov_sha256_final(void *ctx, unsigned char *out, size_t out_size) {
  if (ctx == NULL || out == NULL || out_size < SHA256_DIGEST_LENGTH) {
    return 0;
  }
  return SHA256_Final(out, (SHA256_CTX *)ctx);
}

int awslc_prov_sha256_copy(void *dst, const void *src) {
  if (dst == NULL || src == NULL) {
    return 0;
  }
  *(SHA256_CTX *)dst = *(const SHA256_CTX *)src;
  return 1;
}

// SHA-384

size_t awslc_prov_sha384_ctx_size(void) { return sizeof(SHA512_CTX); }

size_t awslc_prov_sha384_digest_size(void) { return SHA384_DIGEST_LENGTH; }

size_t awslc_prov_sha384_block_size(void) { return SHA384_CBLOCK; }

int awslc_prov_sha384_init(void *ctx) {
  if (ctx == NULL) {
    return 0;
  }
  return SHA384_Init((SHA512_CTX *)ctx);
}

int awslc_prov_sha384_update(void *ctx, const void *data, size_t len) {
  if (ctx == NULL || (data == NULL && len != 0)) {
    return 0;
  }
  return SHA384_Update((SHA512_CTX *)ctx, data, len);
}

int awslc_prov_sha384_final(void *ctx, unsigned char *out, size_t out_size) {
  if (ctx == NULL || out == NULL || out_size < SHA384_DIGEST_LENGTH) {
    return 0;
  }
  return SHA384_Final(out, (SHA512_CTX *)ctx);
}

int awslc_prov_sha384_copy(void *dst, const void *src) {
  if (dst == NULL || src == NULL) {
    return 0;
  }
  *(SHA512_CTX *)dst = *(const SHA512_CTX *)src;
  return 1;
}

// SHA-512

size_t awslc_prov_sha512_ctx_size(void) { return sizeof(SHA512_CTX); }

size_t awslc_prov_sha512_digest_size(void) { return SHA512_DIGEST_LENGTH; }

size_t awslc_prov_sha512_block_size(void) { return SHA512_CBLOCK; }

int awslc_prov_sha512_init(void *ctx) {
  if (ctx == NULL) {
    return 0;
  }
  return SHA512_Init((SHA512_CTX *)ctx);
}

int awslc_prov_sha512_update(void *ctx, const void *data, size_t len) {
  if (ctx == NULL || (data == NULL && len != 0)) {
    return 0;
  }
  return SHA512_Update((SHA512_CTX *)ctx, data, len);
}

int awslc_prov_sha512_final(void *ctx, unsigned char *out, size_t out_size) {
  if (ctx == NULL || out == NULL || out_size < SHA512_DIGEST_LENGTH) {
    return 0;
  }
  return SHA512_Final(out, (SHA512_CTX *)ctx);
}

int awslc_prov_sha512_copy(void *dst, const void *src) {
  if (dst == NULL || src == NULL) {
    return 0;
  }
  *(SHA512_CTX *)dst = *(const SHA512_CTX *)src;
  return 1;
}

// SHA-512/224

size_t awslc_prov_sha512_224_ctx_size(void) { return sizeof(SHA512_CTX); }

size_t awslc_prov_sha512_224_digest_size(void) {
  return SHA512_224_DIGEST_LENGTH;
}

size_t awslc_prov_sha512_224_block_size(void) { return SHA512_CBLOCK; }

int awslc_prov_sha512_224_init(void *ctx) {
  if (ctx == NULL) {
    return 0;
  }
  return SHA512_224_Init((SHA512_CTX *)ctx);
}

int awslc_prov_sha512_224_update(void *ctx, const void *data, size_t len) {
  if (ctx == NULL || (data == NULL && len != 0)) {
    return 0;
  }
  return SHA512_224_Update((SHA512_CTX *)ctx, data, len);
}

int awslc_prov_sha512_224_final(void *ctx, unsigned char *out,
                                size_t out_size) {
  if (ctx == NULL || out == NULL || out_size < SHA512_224_DIGEST_LENGTH) {
    return 0;
  }
  return SHA512_224_Final(out, (SHA512_CTX *)ctx);
}

int awslc_prov_sha512_224_copy(void *dst, const void *src) {
  if (dst == NULL || src == NULL) {
    return 0;
  }
  *(SHA512_CTX *)dst = *(const SHA512_CTX *)src;
  return 1;
}

// SHA-512/256

size_t awslc_prov_sha512_256_ctx_size(void) { return sizeof(SHA512_CTX); }

size_t awslc_prov_sha512_256_digest_size(void) {
  return SHA512_256_DIGEST_LENGTH;
}

size_t awslc_prov_sha512_256_block_size(void) { return SHA512_CBLOCK; }

int awslc_prov_sha512_256_init(void *ctx) {
  if (ctx == NULL) {
    return 0;
  }
  return SHA512_256_Init((SHA512_CTX *)ctx);
}

int awslc_prov_sha512_256_update(void *ctx, const void *data, size_t len) {
  if (ctx == NULL || (data == NULL && len != 0)) {
    return 0;
  }
  return SHA512_256_Update((SHA512_CTX *)ctx, data, len);
}

int awslc_prov_sha512_256_final(void *ctx, unsigned char *out,
                                size_t out_size) {
  if (ctx == NULL || out == NULL || out_size < SHA512_256_DIGEST_LENGTH) {
    return 0;
  }
  return SHA512_256_Final(out, (SHA512_CTX *)ctx);
}

int awslc_prov_sha512_256_copy(void *dst, const void *src) {
  if (dst == NULL || src == NULL) {
    return 0;
  }
  *(SHA512_CTX *)dst = *(const SHA512_CTX *)src;
  return 1;
}
