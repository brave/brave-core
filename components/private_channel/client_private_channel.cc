/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>

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

ChallengeArtefacts ChallengeFirstRound(const char** input,
                                       int input_size,
                                       const char* server_pk) {
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

SecondRoundArtefacts SecondRound(const char* enc_input,
                                 int enc_input_size,
                                 const char* client_sks) {
  struct SecondRoundArtefacts artefacts;
  auto results =
      private_channel::second_round(enc_input, enc_input_size, client_sks);

  artefacts.partial_decryption = convert_to_str(
      results.encoded_partial_dec, results.encoded_partial_dec_size);
  artefacts.proofs =
      convert_to_str(results.encoded_proofs, results.encoded_proofs_size);
  artefacts.rand_vec =
      convert_to_str(results.random_vec, results.random_vec_size);
  artefacts.error = results.error;

  private_channel::free_second_round_result(results);

  return artefacts;
}
