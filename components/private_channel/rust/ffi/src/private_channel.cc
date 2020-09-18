/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "private_channel.h"  // NOLINT
#include <iostream>

extern "C" {
#include "lib.h"  // NOLINT
}

namespace private_channel {

ResultChallenge start_challenge(const char* const* input,
                                int size,
                                const char* server_pk) {
  C_ResultChallenge c_result = client_start_challenge(input, size, server_pk);

  ResultChallenge result;
  result.pkeys = c_result.pkeys;
  result.pkeys_byte_size = c_result.pkeys_byte_size;
  result.skeys = c_result.skeys;
  result.skeys_byte_size = c_result.skeys_byte_size;
  result.key_size = c_result.key_size;
  result.shared_pubkey = c_result.shared_pubkey;
  result.shared_pkeys_byte_size = c_result.shared_pkeys_byte_size;
  result.encrypted_hashes = c_result.encrypted_hashes;
  result.encrypted_hashes_size = c_result.encrypted_hashes_size;
  result.error = c_result.error;

  return result;
}

ResultSecondRound second_round(const char* enc_input,
                               int input_size,
                               const char* sk) {
  C_ResultSecondRound c_result = client_second_round(enc_input, input_size, sk);

  ResultSecondRound result;
  result.encoded_partial_dec = c_result.encoded_partial_dec;
  result.encoded_partial_dec_size = c_result.encoded_partial_dec_size;
  result.encoded_proofs = c_result.encoded_proofs;
  result.encoded_proofs_size = c_result.encoded_proofs_size;
  result.random_vec = c_result.random_vec;
  result.random_vec_size = c_result.random_vec_size;
  result.error = c_result.error;

  return result;
}

void free_first_round_result(ResultChallenge result) {
  C_ResultChallenge c_result;
  c_result.pkeys = result.pkeys;
  c_result.skeys = result.skeys;
  c_result.key_size = result.key_size;
  c_result.shared_pubkey = result.shared_pubkey;
  c_result.encrypted_hashes = result.encrypted_hashes;
  c_result.encrypted_hashes_size = result.encrypted_hashes_size;
  c_result.error = result.error;

  return deallocate_first_round_result(c_result);
}

void free_second_round_result(ResultSecondRound result) {
  C_ResultSecondRound c_result;
  c_result.encoded_partial_dec = result.encoded_partial_dec;
  c_result.encoded_partial_dec_size = result.encoded_partial_dec_size;
  c_result.encoded_proofs = result.encoded_proofs;
  c_result.encoded_proofs_size = result.encoded_proofs_size;
  c_result.random_vec = result.random_vec;
  c_result.random_vec_size = result.random_vec_size;
  c_result.error = result.error;

  return deallocate_second_round_result(c_result);
}

}  // namespace private_channel
