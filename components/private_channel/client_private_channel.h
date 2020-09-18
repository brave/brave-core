/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PRIVATE_CHANNEL_CLIENT_PRIVATE_CHANNEL_H_
#define BRAVE_COMPONENTS_PRIVATE_CHANNEL_CLIENT_PRIVATE_CHANNEL_H_

#include <string>
#include "brave/components/private_channel/rust/ffi/src/private_channel.h"

struct ChallengeArtefacts {
  std::string client_sks;
  std::string client_pks;
  std::string shared_pubkey;
  std::string encrypted_hashes;
  int encrypted_hashes_size;
  bool error;

  ChallengeArtefacts();
  ChallengeArtefacts(const ChallengeArtefacts& other);
  ~ChallengeArtefacts();
};

struct SecondRoundArtefacts {
  std::string partial_decryption;
  std::string proofs;
  std::string rand_vec;
  bool error;

  SecondRoundArtefacts();
  SecondRoundArtefacts(const SecondRoundArtefacts& other);
  ~SecondRoundArtefacts();
};

ChallengeArtefacts ChallengeFirstRound(const char** input,
                                       int input_size,
                                       const char* server_pk_encoded);

SecondRoundArtefacts SecondRound(const char* enc_input,
                                 int size,
                                 const char* client_sks);

#endif  // BRAVE_COMPONENTS_PRIVATE_CHANNEL_CLIENT_PRIVATE_CHANNEL_H_
