/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROPERTIES_BATCH_PROOF_PROPERTIES_H_
#define BRAVELEDGER_PROPERTIES_BATCH_PROOF_PROPERTIES_H_

#include <vector>

#include "bat/ledger/internal/properties/transaction_properties.h"
#include "bat/ledger/internal/properties/ballot_properties.h"

namespace ledger {

struct BatchProofProperties {
  BatchProofProperties();
  BatchProofProperties(
      const BatchProofProperties& properties);
  ~BatchProofProperties();

  bool operator==(
      const BatchProofProperties& rhs) const;

  bool operator!=(
      const BatchProofProperties& rhs) const;

  TransactionProperties transaction;
  BallotProperties ballot;
};

typedef std::vector<BatchProofProperties> BatchProofs;

}  // namespace ledger

#endif  // BRAVELEDGER_PROPERTIES_BATCH_PROOF_PROPERTIES_H_
