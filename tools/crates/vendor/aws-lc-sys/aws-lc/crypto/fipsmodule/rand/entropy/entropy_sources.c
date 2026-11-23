// Copyright Amazon.com Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

#include <openssl/base.h>
#include <openssl/target.h>

#include "internal.h"
#include "../internal.h"
#include "../../delocate.h"
#include "../../../rand_extra/internal.h"
#include "../../../ube/vm_ube_detect.h"

DEFINE_BSS_GET(const struct entropy_source_methods *, entropy_source_methods_override)
DEFINE_BSS_GET(int, allow_entropy_source_methods_override)
DEFINE_STATIC_MUTEX(global_entropy_source_lock)

static int entropy_cpu_get_entropy_multiple8(uint8_t *entropy, size_t entropy_len) {
#if defined(OPENSSL_X86_64)
  if (rdrand_multiple8(entropy, entropy_len) == 1) {
    return 1;
  }
#elif defined(OPENSSL_AARCH64)
  if (rndr_multiple8(entropy, entropy_len) == 1) {
    return 1;
  }
#endif
  return 0;
}

static int entropy_cpu_get_prediction_resistance(
  const struct entropy_source_t *entropy_source,
  uint8_t pred_resistance[RAND_PRED_RESISTANCE_LEN]) {
  return entropy_cpu_get_entropy_multiple8(pred_resistance, RAND_PRED_RESISTANCE_LEN);
}

static int entropy_cpu_get_extra_entropy(
  const struct entropy_source_t *entropy_source,
  uint8_t extra_entropy[CTR_DRBG_ENTROPY_LEN]) {
  return entropy_cpu_get_entropy_multiple8(extra_entropy, CTR_DRBG_ENTROPY_LEN);
}

static int entropy_os_get_extra_entropy(
  const struct entropy_source_t *entropy_source,
  uint8_t extra_entropy[CTR_DRBG_ENTROPY_LEN]) {
  CRYPTO_sysrand(extra_entropy, CTR_DRBG_ENTROPY_LEN);
  return 1;
}

// Tree-DRBG entropy source configuration.
// - Tree DRBG with Jitter Entropy as root for seeding.
// - OS as personalization string source.
// - If run-time is on an x86_64 or Arm64 CPU and it supports rdrand
//   or rndr respectively, use it as a source for prediction resistance.
//   Otherwise, no source.
DEFINE_LOCAL_DATA(struct entropy_source_methods, tree_jitter_entropy_source_methods) {
  out->initialize = tree_jitter_initialize;
  out->zeroize_thread = tree_jitter_zeroize_thread_drbg;
  out->free_thread = tree_jitter_free_thread_drbg;
  out->get_seed = tree_jitter_get_seed;
  out->get_extra_entropy = entropy_os_get_extra_entropy;
  if (have_hw_rng_x86_64() == 1 ||
      have_hw_rng_aarch64() == 1) {
    out->get_prediction_resistance = entropy_cpu_get_prediction_resistance;
  } else {
    out->get_prediction_resistance = NULL;
  }
  out->id = TREE_DRBG_JITTER_ENTROPY_SOURCE;
}

static int opt_out_cpu_jitter_initialize(
  struct entropy_source_t *entropy_source) {
  return 1;
}

static void opt_out_cpu_jitter_zeroize_thread(struct entropy_source_t *entropy_source) {}

static void opt_out_cpu_jitter_free_thread(struct entropy_source_t *entropy_source) {}

static int opt_out_cpu_jitter_get_seed_wrap(
  const struct entropy_source_t *entropy_source, uint8_t seed[CTR_DRBG_ENTROPY_LEN]) {
  return vm_ube_fallback_get_seed(seed);
}

// Define conditions for not using CPU Jitter
static int is_vm_ube_environment(void) {
  return CRYPTO_get_vm_ube_supported();
}

static int has_explicitly_opted_out_of_cpu_jitter(void) {
#if defined(DISABLE_CPU_JITTER_ENTROPY)
  return 1;
#else
  return 0;
#endif
}

static int use_opt_out_cpu_jitter_entropy(void) {
  if (has_explicitly_opted_out_of_cpu_jitter() == 1 ||
      is_vm_ube_environment() == 1) {
    return 1;
  }
  return 0;
}

// Out-out CPU Jitter configurations. CPU source required for rule-of-two.
// - OS as seed source source.
// - Uses rdrand or rndr, if supported, for personalization string. Otherwise
// falls back to OS source.
DEFINE_LOCAL_DATA(struct entropy_source_methods, opt_out_cpu_jitter_entropy_source_methods) {
  out->initialize = opt_out_cpu_jitter_initialize;
  out->zeroize_thread = opt_out_cpu_jitter_zeroize_thread;
  out->free_thread = opt_out_cpu_jitter_free_thread;
  out->get_seed = opt_out_cpu_jitter_get_seed_wrap;
  if (have_hw_rng_x86_64() == 1 ||
      have_hw_rng_aarch64() == 1) {
    out->get_extra_entropy = entropy_cpu_get_extra_entropy;
  } else {
    // Fall back to seed source because a second source must always be present.
    out->get_extra_entropy = opt_out_cpu_jitter_get_seed_wrap;
  }
  out->get_prediction_resistance = NULL;
  out->id = OPT_OUT_CPU_JITTER_ENTROPY_SOURCE;
}

static const struct entropy_source_methods * get_entropy_source_methods(void) {
  if (*allow_entropy_source_methods_override_bss_get() == 1) {
    return *entropy_source_methods_override_bss_get();
  }

  if (use_opt_out_cpu_jitter_entropy()) {
    return opt_out_cpu_jitter_entropy_source_methods();
  }

  return tree_jitter_entropy_source_methods();
}

struct entropy_source_t * get_entropy_source(void) {

  struct entropy_source_t *entropy_source = OPENSSL_zalloc(sizeof(struct entropy_source_t));
  if (entropy_source == NULL) {
    return NULL;
  }

  entropy_source->methods = get_entropy_source_methods();

  // Make sure that the function table contains the minimal number of callbacks
  // that we expect. Also make sure that the entropy source is initialized such
  // that calling code can assume that.
  if (entropy_source->methods == NULL ||
      entropy_source->methods->zeroize_thread == NULL ||
      entropy_source->methods->free_thread == NULL ||
      entropy_source->methods->get_seed == NULL ||
      entropy_source->methods->initialize == NULL ||
      entropy_source->methods->initialize(entropy_source) != 1) {
    OPENSSL_free(entropy_source);
    return NULL;
  }

  return entropy_source;
}

// hw_rng_multiple8_func is the type of a hardware rng wrapper such as
// |CRYPTO_rndr_multiple8| and |CRYPTO_rdrand_multiple8|. It writes |len| bytes
// to |buf| and returns 1 on success, 0 otherwise.
typedef int (*hw_rng_multiple8_func)(uint8_t *buf, size_t len);

// hw_rng_multiple8_with_retry validates |len| and then calls |hw_rng| until it
// succeeds or |max_attempts| calls have been made. |max_attempts| must be
// positive.
// A hardware rng wrapper will typically execute the underlying instruction
// multiple times and a failing call can therefore leave a prefix of |buf|
// written. This is not an issue, because the retry re-generates the entire
// |buf| and the contents of |buf| are only consumed on success. Retrying the
// entire request, instead of only the failed instruction execution, is easier
// to implement on the C-level and it should be a very rare event.
// Outputs 1 on success, 0 otherwise.
static int hw_rng_multiple8_with_retry(hw_rng_multiple8_func hw_rng,
  uint8_t *buf, size_t len, size_t max_attempts) {

  if (len == 0 || ((len & 0x7) != 0)) {
    return 0;
  }

  for (size_t attempts = 0; attempts < max_attempts; attempts++) {
    if (hw_rng(buf, len) == 1) {
      return 1;
    }
  }

  return 0;
}

int hw_rng_multiple8_with_retry_FOR_TESTING(
  int (*hw_rng)(uint8_t *buf, size_t len), uint8_t *buf, size_t len,
  size_t max_attempts) {
  return hw_rng_multiple8_with_retry(hw_rng, buf, len, max_attempts);
}

// rndr_multiple8 should only be called if |have_hw_rng_aarch64| returned true.
int rndr_multiple8(uint8_t *buf, const size_t len) {
  return hw_rng_multiple8_with_retry(CRYPTO_rndr_multiple8, buf, len,
                                     RNDR_MAX_ATTEMPTS);
}

int have_hw_rng_aarch64_for_testing(void) {
  return have_hw_rng_aarch64();
}

// rdrand_multiple8 should only be called if |have_hw_rng_x86_64| returned true.
int rdrand_multiple8(uint8_t *buf, size_t len) {
  return hw_rng_multiple8_with_retry(CRYPTO_rdrand_multiple8, buf, len,
                                     RDRAND_MAX_ATTEMPTS);
}

int have_hw_rng_x86_64_for_testing(void) {
  return have_hw_rng_x86_64();
}

void override_entropy_source_method_FOR_TESTING(
  const struct entropy_source_methods *override_entropy_source_methods) {

  CRYPTO_STATIC_MUTEX_lock_write(global_entropy_source_lock_bss_get());
  *allow_entropy_source_methods_override_bss_get() = 1;
  *entropy_source_methods_override_bss_get() = override_entropy_source_methods;
  CRYPTO_STATIC_MUTEX_unlock_write(global_entropy_source_lock_bss_get());
}

int get_entropy_source_method_id_FOR_TESTING(void) {
  int id;
  CRYPTO_STATIC_MUTEX_lock_read(global_entropy_source_lock_bss_get());
  const struct entropy_source_methods *entropy_source_method = get_entropy_source_methods();
  id = entropy_source_method->id;
  CRYPTO_STATIC_MUTEX_unlock_read(global_entropy_source_lock_bss_get());
  return id;
}
