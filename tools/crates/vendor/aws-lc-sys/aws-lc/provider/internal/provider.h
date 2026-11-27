// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

#ifndef AWSLC_PROVIDER_PROVIDER_H
#define AWSLC_PROVIDER_PROVIDER_H

// Shared between the provider's OpenSSL-facing files.

#include <stdint.h>

#include <openssl/core.h>
#include <openssl/core_dispatch.h>

#include "internal/backend.h"

#if defined(__cplusplus)
extern "C" {
#endif

// The property strings every algorithm we register carries.
#define AWSLC_PROV_PROPERTIES "provider=awslc"
// Apply fips=yes to algorithms that AWS-LC can approve in FIPS mode.
#define AWSLC_PROV_FIPS_PROPERTIES AWSLC_PROV_PROPERTIES ",fips=yes"

// AWSLC_PROV_CTX is the per-load provider context.
typedef struct awslc_prov_ctx_st AWSLC_PROV_CTX;

// The core handle for this load, used when raising errors back to the core.
const OSSL_CORE_HANDLE *awslc_prov_ctx_handle(const AWSLC_PROV_CTX *ctx);

// The core upcalls this provider uses, captured at init.
typedef struct {
  // OPENSSL_CORE_CTX *(const OSSL_CORE_HANDLE *prov)
  // The opaque core context for this load, which the indicator upcall is
  // addressed to.
  OSSL_FUNC_core_get_libctx_fn *get_libctx;

  // void (OPENSSL_CORE_CTX *ctx, OSSL_INDICATOR_CALLBACK **cb)
  // Fetches the application's FIPS indicator callback, or leaves |cb| NULL.
  OSSL_FUNC_indicator_cb_fn *indicator_cb;

  // void (const OSSL_CORE_HANDLE *prov)
  // Opens a new record on the caller's error queue.
  OSSL_FUNC_core_new_error_fn *new_error;

  // void (const OSSL_CORE_HANDLE *prov, const char *file, int line,
  //       const char *func)
  // Attaches origin metadata to the open record.
  OSSL_FUNC_core_set_error_debug_fn *set_error_debug;

  // void (const OSSL_CORE_HANDLE *prov, uint32_t reason, const char *fmt,
  //       va_list args)
  // Sets the reason and formatted detail on the open record. Unlike
  // the other two error upcalls it reads |prov| to find the error library to file
  // under, so it cannot be passed NULL.
  OSSL_FUNC_core_vset_error_fn *vset_error;
} AWSLC_PROV_UPCALLS;

// NULL for a NULL |ctx|, which is what makes raising without a context a no-op
// rather than a crash.
const AWSLC_PROV_UPCALLS *awslc_prov_ctx_upcalls(const AWSLC_PROV_CTX *ctx);

// Whether the linked AWS-LC reported FIPS mode when this provider initialized.
int awslc_prov_ctx_is_fips(const AWSLC_PROV_CTX *ctx);

// Looks up the application's FIPS indicator callback. If one is registered,
// calls it with |type| and |description| and returns its result as 0 or 1;
// otherwise returns 1.
int awslc_prov_ctx_run_fips_callback(const AWSLC_PROV_CTX *ctx,
                                     const char *type,
                                     const char *description);

// The fan-out, implemented in registry.c and published by provider.c in the
// top-level dispatch table. The dispatch tables it hands out are declared in
// per-class headers, which only registry.c and the class implementations include.
const OSSL_ALGORITHM *awslc_prov_query_operation(void *provctx, int operation_id,
                                                 int *no_store);

// Error reporting, implemented in errors.c. The reason namespace is in
// internal/backend.h.

// The reason-string table for the provider's private error library, published by
// provider.c in the top-level dispatch table.
OSSL_FUNC_provider_get_reason_strings_fn awslc_prov_get_reason_strings;

// Raise one error from the provider layer itself. |detail| is optional
// per-occurrence text.
void awslc_prov_error_raise(const AWSLC_PROV_CTX *ctx, uint32_t reason,
                            const char *file, int line, const char *detail);

// Raise at the call site's own file and line.
#define AWSLC_PROV_ERROR_RAISE(ctx, reason, detail) \
  awslc_prov_error_raise((ctx), (reason), __FILE__, __LINE__, (detail))

// Every dispatch slot that calls into AWS-LC brackets those calls in one call:
//
//   AWSLC_PROV_CALL call;
//   awslc_prov_call_begin(&call, ctx->provctx);
//   ok = <one or more backend calls>;
//   if (!AWSLC_PROV_CALL_END(&call, ok, ctx->algorithm_name)) {
//     <release or wipe anything the backend calls produced>
//     return 0;
//   }
//
// One bracket per dispatch call, never nested: begin empties AWS-LC's queue, so
// an inner bracket would destroy the outer call's records.
//
// A dispatch call crosses into AWS-LC in one of two ways, and closes its
// bracket to match:
//
//   AWSLC_PROV_CALL_END          Setup and intermediate steps: context
//                                allocation, init, update, copy, self-test.
//                                AWS-LC attests nothing here.
//   AWSLC_PROV_CALL_END_CHECK_APPROVED
//                                The step that completes an AWS-LC approved
//                                service and produces its result: digest
//                                final, keygen, sign, verify, derive. AWS-LC
//                                bumps its FIPS service-indicator counter only
//                                here, and only when the service ran approved.
//
// AWS-LC's service indicator tests
// (crypto/fipsmodule/service_indicator/service_indicator_test.cc) are the
// authority on which calls increment the indicator.
typedef struct {
  const AWSLC_PROV_CTX *provctx;
  uint64_t indicator;
} AWSLC_PROV_CALL;

void awslc_prov_call_begin(AWSLC_PROV_CALL *call,
                           const AWSLC_PROV_CTX *provctx);

// Returns |succeeded| as 0 or 1. The AWS-LC queue is left empty either way:
//
//   succeeded  Discard AWS-LC's records, which it queues on recoverable
//              internal paths.
//   failed     Translate AWS-LC's records onto OpenSSL's queue, oldest first,
//              or raise BACKEND_ERROR with the optional |detail| if AWS-LC
//              queued none, so a failed call never leaves the queue empty.
int awslc_prov_call_end(AWSLC_PROV_CALL *call, int succeeded,
                        const char *detail, const char *file, int line);

// As awslc_prov_call_end, with |type| as the fallback detail, and returns 1
// only if the caller may publish its output. On success an unapproved verdict
// clears |*approved| and, in FIPS mode, passes |type| and |description| to the
// application's indicator callback, whose veto raises UNAPPROVED_OPERATION and
// returns 0. |type|, |description|, and |approved| must be non-NULL.
//
// |type| and |description| are the two strings OpenSSL's
// OSSL_INDICATOR_CALLBACK receives: free-form labels for the algorithm (e.g.
// "SHA2-256") and the operation (e.g. "digest").
int awslc_prov_call_end_check_approved(AWSLC_PROV_CALL *call, int succeeded,
                                       const char *type,
                                       const char *description, int *approved,
                                       const char *file, int line);

#define AWSLC_PROV_CALL_END(call, succeeded, detail) \
  awslc_prov_call_end((call), (succeeded), (detail), __FILE__, __LINE__)

#define AWSLC_PROV_CALL_END_CHECK_APPROVED(call, succeeded, type, description, \
                                           approved)                           \
  awslc_prov_call_end_check_approved((call), (succeeded), (type),              \
                                     (description), (approved), __FILE__,      \
                                     __LINE__)

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // AWSLC_PROVIDER_PROVIDER_H
