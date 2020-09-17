/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "private_channel.hpp"
#include <iostream>

extern "C" {
#include "lib.h"  // NOLINT
}

namespace private_channel {

  ResultChallenge start_challenge(
    const char* const* input_ptr, int size, const char* server_pk) {
      C_ResultChallenge c_result =
        client_start_challenge(input_ptr, size, server_pk);

      ResultChallenge result;
      result.pkey_ptr = c_result.pkey_ptr;
      result.skey_ptr = c_result.skey_ptr;
      result.key_size = c_result.key_size;
      result.shared_pubkey_ptr = c_result.shared_pubkey_ptr;
      result.encrypted_hashes_ptr = c_result.encrypted_hashes_ptr;
      result.encrypted_hashes_size = c_result.encrypted_hashes_size;
      result.error = c_result.error;

      return result;
  }

  ResultSecondRound second_round(
    const char* enc_input_ptr, int input_size, const char*  sk) {
      C_ResultSecondRound c_result =
        client_second_round(enc_input_ptr, input_size, sk);

      ResultSecondRound result;
      result.encoded_partial_dec_ptr = c_result.encoded_partial_dec_ptr;
      result.encoded_partial_dec_size = c_result.encoded_partial_dec_size;
      result.encoded_proofs_ptr = c_result.encoded_proofs_ptr;
      result.encoded_proofs_size = c_result.encoded_proofs_size;
      result.random_vec_ptr = c_result.random_vec_ptr;
      result.random_vec_size = c_result.random_vec_size;
      result.error = c_result.error;

      return result;
  }

  void free_first_round_result(ResultChallenge result) {
    C_ResultChallenge c_result;
    c_result.pkey_ptr = result.pkey_ptr;
    c_result.skey_ptr = result.skey_ptr;
    c_result.key_size = result.key_size;
    c_result.shared_pubkey_ptr = result.shared_pubkey_ptr;
    c_result.encrypted_hashes_ptr = result.encrypted_hashes_ptr;
    c_result.encrypted_hashes_size = result.encrypted_hashes_size;
    c_result.error = result.error;

    return deallocate_first_round_result(c_result);
  }

  void free_second_round_result(ResultSecondRound result) {
    C_ResultSecondRound c_result;
    c_result.encoded_partial_dec_ptr = result.encoded_partial_dec_ptr;
    c_result.encoded_partial_dec_size = result.encoded_partial_dec_size;
    c_result.encoded_proofs_ptr = result.encoded_proofs_ptr;
    c_result.encoded_proofs_size = result.encoded_proofs_size;
    c_result.random_vec_ptr = result.random_vec_ptr;
    c_result.random_vec_size = result.random_vec_size;
    c_result.error = result.error;

    return deallocate_second_round_result(c_result);
  }

}  // namespace private_channel
