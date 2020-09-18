/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef BRAVE_COMPONENTS_PRIVATE_CHANNEL_RUST_FFI_SRC_LIB_H_
#define BRAVE_COMPONENTS_PRIVATE_CHANNEL_RUST_FFI_SRC_LIB_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define C_KEY_SIZE 32

typedef struct {
  const uint8_t* encoded_partial_dec;
  uintptr_t encoded_partial_dec_size;
  const uint8_t* encoded_proofs;
  uintptr_t encoded_proofs_size;
  const uint8_t* random_vec;
  uintptr_t random_vec_size;
  bool error;
} C_ResultSecondRound;

typedef struct {
  const uint8_t* pkeys;
  uintptr_t pkeys_byte_size;
  const uint8_t* skeys;
  uintptr_t skeys_byte_size;
  const uint8_t* shared_pubkey;
  uintptr_t shared_pkeys_byte_size;
  const uint8_t* encrypted_hashes;
  uintptr_t encrypted_hashes_size;
  uintptr_t key_size;
  bool error;
} C_ResultChallenge;

C_ResultSecondRound client_second_round(const char* input,
                                        int _input_size,
                                        const char* client_sk_encoded);

/**
 * Starts client attestation challenge;
 */
C_ResultChallenge client_start_challenge(const char* const* input,
                                         int input_size,
                                         const char* server_pk_encoded);

void deallocate_first_round_result(C_ResultChallenge result);

void deallocate_second_round_result(C_ResultSecondRound result);

#endif /* BRAVE_COMPONENTS_PRIVATE_CHANNEL_RUST_FFI_SRC_LIB_H_ */
