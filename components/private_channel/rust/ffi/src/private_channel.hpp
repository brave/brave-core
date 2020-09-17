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
#include "lib.h"
}

namespace private_channel {

	typedef struct {
		const uint8_t *pkey_ptr;
		const uint8_t *skey_ptr;
		uintptr_t key_size;
		const uint8_t *shared_pubkey_ptr;
		const uint8_t *encrypted_hashes_ptr;
		uintptr_t encrypted_hashes_size;
		bool error;
	} ResultChallenge;

	typedef struct {
		const uint8_t *encoded_partial_dec_ptr;
		uintptr_t encoded_partial_dec_size;
		const uint8_t *encoded_proofs_ptr;
		uintptr_t encoded_proofs_size;
		const uint8_t *random_vec_ptr;
		uintptr_t random_vec_size;
		bool error;
	} ResultSecondRound;

  ResultChallenge start_challenge(
    const char* const* input_ptr, int size, const char* server_pk);
	
  ResultSecondRound second_round(
    const char* enc_input_ptr, int size,  const char* sk);

  void free_first_round_result(ResultChallenge result);

  void free_second_round_result(ResultSecondRound result);

}

#endif	// BRAVE_COMPONENTS_PRIVATE_CHANNEL_RUST_FFI_SRC_PRIVATE_CHANNEL_H_
