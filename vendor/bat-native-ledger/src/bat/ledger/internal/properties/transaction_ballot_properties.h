/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROPERTIES_TRANSACTION_BALLOT_PROPERTIES_H_
#define BRAVELEDGER_PROPERTIES_TRANSACTION_BALLOT_PROPERTIES_H_

#include <stdint.h>
#include <string>
#include <vector>

namespace ledger {

struct TransactionBallotProperties {
  TransactionBallotProperties();
  TransactionBallotProperties(
      const TransactionBallotProperties& properties);
  ~TransactionBallotProperties();

  bool operator==(
      const TransactionBallotProperties& rhs) const;

  bool operator!=(
      const TransactionBallotProperties& rhs) const;

  std::string publisher;
  uint32_t count;
};

typedef std::vector<TransactionBallotProperties> TransactionBallots;

}  // namespace ledger

#endif  // BRAVELEDGER_PROPERTIES_TRANSACTION_BALLOT_PROPERTIES_H_
