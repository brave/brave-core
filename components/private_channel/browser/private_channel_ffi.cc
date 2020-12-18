/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <vector>

#include "brave/components/private_channel/browser/private_channel_ffi.h"
#include "brave/components/private_channel/browser/utils.h"
#include "brave/components/private_channel/rust/ffi/src/private_channel.h"

namespace private_channel {

ChallengeArtifacts::ChallengeArtifacts() {}
ChallengeArtifacts::ChallengeArtifacts(const ChallengeArtifacts& other) =
    default;
ChallengeArtifacts::~ChallengeArtifacts() {}

SecondRoundArtifacts::SecondRoundArtifacts() {}
SecondRoundArtifacts::SecondRoundArtifacts(const SecondRoundArtifacts& other) =
    default;
SecondRoundArtifacts::~SecondRoundArtifacts() {}

ChallengeArtifacts ChallengeFirstRound(std::string server_pk_str) {
  // TODO(@gpestana): finish
  std::string s = "";
  std::vector<std::string> input_vec = {s, s, s, s, s, s, s, s, s};

  const char* server_pk = server_pk_str.c_str();

  int input_size = input_vec.size();
  const char* input[input_size];

  for (std::size_t i = 0; i < input_vec.size(); ++i) {
    input[i] = input_vec[i].c_str();
  }

  struct ChallengeArtifacts artifacts;
  auto results = start_challenge(input, input_size, server_pk);

  artifacts.client_pks =
      convert_to_str_array(results.pkeys, results.pkeys_byte_size);
  artifacts.client_sks =
      convert_to_str_array(results.skeys, results.skeys_byte_size);
  artifacts.shared_pubkey = convert_to_str_array(
      results.shared_pubkey, results.shared_pkeys_byte_size);
  artifacts.encrypted_hashes = convert_to_str_array(
      results.encrypted_hashes, results.encrypted_hashes_size);
  artifacts.encrypted_hashes_size = results.encrypted_hashes_size;
  artifacts.error = results.error;

  free_first_round_result(results);

  return artifacts;
}

SecondRoundArtifacts SecondRound(std::string enc_input_str,
                                 int enc_input_size,
                                 std::string client_sks_str) {
  const char* enc_input = enc_input_str.c_str();
  const char* client_sks = client_sks_str.c_str();

  struct SecondRoundArtifacts artifacts;
  auto results = second_round(enc_input, enc_input_size, client_sks);

  artifacts.partial_decryption = convert_to_str_array(
      results.encoded_partial_dec, results.encoded_partial_dec_size);
  artifacts.dec_proofs = convert_to_str_array(results.encoded_proofs_rand,
                                              results.encoded_proofs_rand_size);
  artifacts.proofs = convert_to_str_array(results.encoded_proofs_dec,
                                          results.encoded_proofs_dec_size);
  artifacts.rand_vec =
      convert_to_str_array(results.random_vec, results.random_vec_size);
  artifacts.error = results.error;

  free_second_round_result(results);

  return artifacts;
}

}  // namespace private_channel
