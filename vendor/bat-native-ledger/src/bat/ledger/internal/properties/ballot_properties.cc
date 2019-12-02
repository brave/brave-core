/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/properties/ballot_properties.h"

namespace ledger {

BallotProperties::BallotProperties() = default;

BallotProperties::BallotProperties(
    const BallotProperties& properties) {
  viewing_id = properties.viewing_id;
  surveyor_id = properties.surveyor_id;
  publisher = properties.publisher;
  count = properties.count;
  prepare_ballot = properties.prepare_ballot;
  proof_ballot = properties.proof_ballot;
}

BallotProperties::~BallotProperties() = default;

bool BallotProperties::operator==(
    const BallotProperties& rhs) const {
  return viewing_id == rhs.viewing_id &&
      surveyor_id == rhs.surveyor_id &&
      publisher == rhs.publisher &&
      count == rhs.count &&
      prepare_ballot == rhs.prepare_ballot &&
      proof_ballot == rhs.proof_ballot;
}

bool BallotProperties::operator!=(
    const BallotProperties& rhs) const {
  return !(*this == rhs);
}

}  // namespace ledger
