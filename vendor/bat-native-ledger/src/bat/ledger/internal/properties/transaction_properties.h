/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROPERTIES_TRANSACTION_PROPERTIES_H_
#define BRAVELEDGER_PROPERTIES_TRANSACTION_PROPERTIES_H_

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

#include "bat/ledger/internal/properties/transaction_ballot_properties.h"

namespace ledger {

struct TransactionProperties {
  TransactionProperties();
  TransactionProperties(
      const TransactionProperties& properties);
  ~TransactionProperties();

  bool operator==(
      const TransactionProperties& rhs) const;

  bool operator!=(
      const TransactionProperties& rhs) const;

  std::string viewing_id;
  std::string surveyor_id;
  std::map<std::string, double> contribution_rates;
  std::string contribution_probi;
  std::string submission_timestamp;
  std::string anonize_viewing_id;
  std::string registrar_vk;
  std::string master_user_token;
  std::vector<std::string> surveyor_ids;
  uint32_t vote_count;
  TransactionBallots transaction_ballots;
};

typedef std::vector<TransactionProperties> Transactions;

}  // namespace ledger

#endif  // BRAVELEDGER_PROPERTIES_TRANSACTION_PROPERTIES_H_
