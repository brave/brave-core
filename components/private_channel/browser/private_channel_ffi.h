/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PRIVATE_CHANNEL_BROWSER_PRIVATE_CHANNEL_FFI_H_
#define BRAVE_COMPONENTS_PRIVATE_CHANNEL_BROWSER_PRIVATE_CHANNEL_FFI_H_

#include <string>

#include "brave/components/private_channel/rust/ffi/src/private_channel.h"

namespace private_channel {

struct ChallengeArtifacts {
  std::string client_sks;
  std::string client_pks;
  std::string shared_pubkey;
  std::string encrypted_hashes;
  int encrypted_hashes_size;
  bool error;

  ChallengeArtifacts();
  ChallengeArtifacts(const ChallengeArtifacts& other);
  ~ChallengeArtifacts();
};

struct SecondRoundArtifacts {
  std::string partial_decryption;
  std::string proofs;
  std::string dec_proofs;
  std::string rand_vec;
  bool error;

  SecondRoundArtifacts();
  SecondRoundArtifacts(const SecondRoundArtifacts& other);
  ~SecondRoundArtifacts();
};

ChallengeArtifacts ChallengeFirstRound(std::string server_pk_str);

SecondRoundArtifacts SecondRound(std::string enc_input_str,
                                 int size,
                                 std::string client_sks_str);

}  // namespace private_channel

#endif  // BRAVE_COMPONENTS_PRIVATE_CHANNEL_BROWSER_PRIVATE_CHANNEL_FFI_H_
