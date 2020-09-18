/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PRIVATE_CHANNEL_RUST_FFI_SRC_PRIVATE_CHANNEL_H_
#define BRAVE_COMPONENTS_PRIVATE_CHANNEL_RUST_FFI_SRC_PRIVATE_CHANNEL_H_
#include <memory>
#include <string>
#include <vector>

extern "C" {
#include "lib.h"  // NOLINT
}

namespace private_channel {

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
} ResultChallenge;

typedef struct {
  const uint8_t* encoded_partial_dec;
  uintptr_t encoded_partial_dec_size;
  const uint8_t* encoded_proofs;
  uintptr_t encoded_proofs_size;
  const uint8_t* random_vec;
  uintptr_t random_vec_size;
  bool error;
} ResultSecondRound;

ResultChallenge start_challenge(const char* const* input,
                                int size,
                                const char* server_pk);

ResultSecondRound second_round(const char* enc_input, int size, const char* sk);

void free_first_round_result(ResultChallenge result);

void free_second_round_result(ResultSecondRound result);

}  // namespace private_channel

#endif  // BRAVE_COMPONENTS_PRIVATE_CHANNEL_RUST_FFI_SRC_PRIVATE_CHANNEL_H_
