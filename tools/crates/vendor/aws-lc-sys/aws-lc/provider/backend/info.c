// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

#include <stdio.h>
#include <stdlib.h>

#include <openssl/crypto.h>
#include <openssl/service_indicator.h>

#include "internal/backend.h"

const char *awslc_prov_backend_version(void) {
  return awslc_version_string();
}

int awslc_prov_backend_is_fips(void) {
  return FIPS_mode();
}

uint64_t awslc_prov_service_indicator_before_call(void) {
  return FIPS_service_indicator_before_call();
}

int awslc_prov_service_indicator_after_call(uint64_t before) {
  const uint64_t after = FIPS_service_indicator_after_call();

  // Non-FIPS AWS-LC reports every call approved; that must not reach OpenSSL.
  return FIPS_mode() && before != after;
}

int awslc_prov_backend_self_test(void) {
  if (BORINGSSL_self_test()) {
    return 1;
  }
  if (FIPS_mode()) {
    fprintf(stderr, "AWS-LC FIPS failure caused by:\non-demand self-test failed\n");
    fflush(stderr);
    abort();
  }
  return 0;
}
