/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <vector>

#include "base/logging.h"

#include "brave/components/private_channel/client_private_channel.h"
#include "brave/components/private_channel/rust/ffi/src/private_channel.h"
#include "brave/components/private_channel/utils.h"

ChallengeArtefacts::ChallengeArtefacts() {}
ChallengeArtefacts::ChallengeArtefacts(const ChallengeArtefacts& other) =
    default;
ChallengeArtefacts::~ChallengeArtefacts() {}

SecondRoundArtefacts::SecondRoundArtefacts() {}
SecondRoundArtefacts::SecondRoundArtefacts(const SecondRoundArtefacts& other) =
    default;
SecondRoundArtefacts::~SecondRoundArtefacts() {}

ChallengeArtefacts ChallengeFirstRound(std::string server_pk_str) {
  // TODO(@gpestana): finish
  std::string s = "";
  std::vector<std::string> input_vec = {s, s, s, s, s, s, s, s, s};

  const char* server_pk = server_pk_str.c_str();

  int input_size = input_vec.size();
  const char* input[input_size];

  for (std::size_t i = 0; i < input_vec.size(); ++i) {
    input[i] = input_vec[i].c_str();
  }

  struct ChallengeArtefacts artefacts;
  auto results = private_channel::start_challenge(input, input_size, server_pk);

  artefacts.client_pks = convert_to_str(results.pkeys, results.pkeys_byte_size);
  artefacts.client_sks = convert_to_str(results.skeys, results.skeys_byte_size);
  artefacts.shared_pubkey =
      convert_to_str(results.shared_pubkey, results.shared_pkeys_byte_size);
  artefacts.encrypted_hashes =
      convert_to_str(results.encrypted_hashes, results.encrypted_hashes_size);
  artefacts.encrypted_hashes_size = results.encrypted_hashes_size;
  artefacts.error = results.error;

  private_channel::free_first_round_result(results);

  return artefacts;
}

SecondRoundArtefacts SecondRound(std::string enc_input_str,
                                 int enc_input_size,
                                 std::string client_sks_str) {
  const char* enc_input = enc_input_str.c_str();
  const char* client_sks = client_sks_str.c_str();

  struct SecondRoundArtefacts artefacts;
  auto results =
      private_channel::second_round(enc_input, enc_input_size, client_sks);

  artefacts.partial_decryption = convert_to_str(
      results.encoded_partial_dec, results.encoded_partial_dec_size);
  artefacts.dec_proofs = convert_to_str(results.encoded_proofs_rand,
                                        results.encoded_proofs_rand_size);
  artefacts.proofs = convert_to_str(results.encoded_proofs_dec,
                                    results.encoded_proofs_dec_size);
  artefacts.rand_vec =
      convert_to_str(results.random_vec, results.random_vec_size);
  artefacts.error = results.error;

  private_channel::free_second_round_result(results);

  return artefacts;
}
