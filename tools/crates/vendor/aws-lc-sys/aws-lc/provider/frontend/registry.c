// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

#include <openssl/core.h>
#include <openssl/core_dispatch.h>

#include "internal/frontend/digests.h"
#include "internal/provider.h"

// A macro for defining algorithm rows for the OpenSSL provider dispatch table.
//
// |names| are provided as ':' separated values and come verbatim from OpenSSL's
// providers/implementations/include/prov/names.h
// They include legacy spellings, aliases, and the OID.
//
// |property_definition| is the row's property string.
//
// |algorithm| is the symbol-name stem for each specific algorithm's macro
// defined symbols and should be given in lower_snake_case.
//
// |description| is a simple string description for the algorithm.
#define AWSLC_PROV_ALG(names, property_definition, algorithm, description) \
  {names, property_definition, awslc_prov_##algorithm##_functions, description},

// The digest rows, expanded once per table below. |fips_property_definition| is
// given to every algorithm AWS-LC can approve; a row AWS-LC can never approve
// uses AWSLC_PROV_PROPERTIES directly. Each row cites the names.h macro it
// copies.
#define AWSLC_PROV_DIGESTS(fips_property_definition)                           \
  /* PROV_NAMES_SHA2_224 */                                                    \
  AWSLC_PROV_ALG("SHA2-224:SHA-224:SHA224:2.16.840.1.101.3.4.2.4",             \
                 fips_property_definition, sha224,                             \
                 "AWS-LC SHA2-224 implementation")                             \
  /* PROV_NAMES_SHA2_256 */                                                    \
  AWSLC_PROV_ALG("SHA2-256:SHA-256:SHA256:2.16.840.1.101.3.4.2.1",             \
                 fips_property_definition, sha256,                             \
                 "AWS-LC SHA2-256 implementation")                             \
  /* PROV_NAMES_SHA2_384 */                                                    \
  AWSLC_PROV_ALG("SHA2-384:SHA-384:SHA384:2.16.840.1.101.3.4.2.2",             \
                 fips_property_definition, sha384,                             \
                 "AWS-LC SHA2-384 implementation")                             \
  /* PROV_NAMES_SHA2_512 */                                                    \
  AWSLC_PROV_ALG("SHA2-512:SHA-512:SHA512:2.16.840.1.101.3.4.2.3",             \
                 fips_property_definition, sha512,                             \
                 "AWS-LC SHA2-512 implementation")                             \
  /* PROV_NAMES_SHA2_512_224 */                                                \
  AWSLC_PROV_ALG("SHA2-512/224:SHA-512/224:SHA512-224:2.16.840.1.101.3.4.2.5", \
                 fips_property_definition, sha512_224,                         \
                 "AWS-LC SHA2-512/224 implementation")                         \
  /* PROV_NAMES_SHA2_512_256 */                                                \
  AWSLC_PROV_ALG("SHA2-512/256:SHA-512/256:SHA512-256:2.16.840.1.101.3.4.2.6", \
                 fips_property_definition, sha512_256,                         \
                 "AWS-LC SHA2-512/256 implementation")

static const OSSL_ALGORITHM awslc_prov_digests_fips[] = {
    AWSLC_PROV_DIGESTS(AWSLC_PROV_FIPS_PROPERTIES)
    {NULL, NULL, NULL, NULL}};
static const OSSL_ALGORITHM awslc_prov_digests[] = {
    AWSLC_PROV_DIGESTS(AWSLC_PROV_PROPERTIES)
    {NULL, NULL, NULL, NULL}};

const OSSL_ALGORITHM *awslc_prov_query_operation(void *provctx,
                                                 int operation_id,
                                                 int *no_store) {
  // FIPS mode is fixed for the process, so the core may cache either table.
  if (no_store != NULL) {
    *no_store = 0;
  }

  switch (operation_id) {
    case OSSL_OP_DIGEST:
      return awslc_prov_ctx_is_fips(provctx) ? awslc_prov_digests_fips
                                             : awslc_prov_digests;
    default:
      // Returning NULL is what lets the fetch fall through to another provider
      // rather than fail, so every class we do not serve must land here.
      return NULL;
  }
}
