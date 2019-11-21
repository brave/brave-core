/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/properties/transaction_ballot_properties.h"

namespace ledger {

TransactionBallotProperties::TransactionBallotProperties()
    : count(0) {}

TransactionBallotProperties::TransactionBallotProperties(
    const TransactionBallotProperties& properties) {
  publisher = properties.publisher;
  count = properties.count;
}

TransactionBallotProperties::~TransactionBallotProperties() = default;

bool TransactionBallotProperties::operator==(
    const TransactionBallotProperties& rhs) const {
  return publisher == rhs.publisher &&
      count == rhs.count;
}

bool TransactionBallotProperties::operator!=(
    const TransactionBallotProperties& rhs) const {
  return !(*this == rhs);
}

}  // namespace ledger
