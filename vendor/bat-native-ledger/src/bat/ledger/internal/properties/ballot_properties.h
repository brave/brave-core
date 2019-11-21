/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROPERTIES_BALLOT_PROPERTIES_H_
#define BRAVELEDGER_PROPERTIES_BALLOT_PROPERTIES_H_

#include <stdint.h>
#include <string>
#include <vector>

namespace ledger {

struct BallotProperties {
  BallotProperties();
  BallotProperties(
      const BallotProperties& properties);
  ~BallotProperties();

  bool operator==(
      const BallotProperties& rhs) const;

  bool operator!=(
      const BallotProperties& rhs) const;

  std::string viewing_id;
  std::string surveyor_id;
  std::string publisher;
  uint32_t count;
  std::string prepare_ballot;
  std::string proof_ballot;
};

typedef std::vector<BallotProperties> Ballots;

}  // namespace ledger

#endif  // BRAVELEDGER_PROPERTIES_BALLOT_PROPERTIES_H_
