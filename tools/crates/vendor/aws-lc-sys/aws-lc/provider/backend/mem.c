// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

// Allocation, deliberately on AWS-LC's allocator rather than the core's memory
// upcalls: routing through OpenSSL's allocator would entangle the two libcryptos'
// memory management for no benefit. Only caller-owned OSSL_PARAM buffers cross
// the boundary, and those are never allocated by us.

#include <openssl/mem.h>

#include "internal/backend.h"

void *awslc_prov_zalloc(size_t size) {
  return OPENSSL_zalloc(size);
}

void awslc_prov_clear_free(void *ptr, size_t size) {
  OPENSSL_clear_free(ptr, size);
}

void awslc_prov_cleanse(void *ptr, size_t size) {
  OPENSSL_cleanse(ptr, size);
}
