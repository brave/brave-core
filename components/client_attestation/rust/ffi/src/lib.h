/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CLIENT_ATTESTATION_RUST_FFI_H
#define CLIENT_ATTESTATION_RUST_FFI_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  const uint8_t *pkey_ptr;
  const uint8_t *skey_ptr;
  uintptr_t key_size;
  const uint8_t *shared_pubkey_ptr;
  const uint8_t *encrypted_hashes_ptr;
  uintptr_t encrypted_hashes_size;
  bool error;
} C_ResultChallenge;

typedef struct {
  const uint8_t *encoded_partial_dec_ptr;
  uintptr_t encoded_partial_dec_size;
  const uint8_t *encoded_proofs_ptr;
  uintptr_t encoded_proofs_size;
  const uint8_t *random_vec_ptr;
  uintptr_t random_vec_size;
  bool error;
} C_ResultSecondRound;


C_ResultChallenge client_start_challenge(const char *const *input,
                                         int input_size,
                                         const uint8_t *server_pk_encoded);

C_ResultSecondRound client_second_round(const uint8_t *input,
                                        int input_size,
                                        const uint8_t *client_sk_encoded);

#endif /* CLIENT_ATTESTATION_RUST_FFI_H */
