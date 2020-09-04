/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_REWARDS_REWARDS_SERVER_H_
#define BRAVELEDGER_ENDPOINT_REWARDS_REWARDS_SERVER_H_

#include <memory>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/endpoint/rewards/get_prefix_list/get_prefix_list.h"

namespace ledger {
class LedgerImpl;

namespace endpoint {

class RewardsServer {
 public:
  explicit RewardsServer(LedgerImpl* ledger);
  ~RewardsServer();

  rewards::GetPrefixList* get_prefix_list() const;

 private:
  std::unique_ptr<rewards::GetPrefixList> get_prefix_list_;
};

}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_REWARDS_REWARDS_SERVER_H_
