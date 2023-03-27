/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_AC_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_AC_H_

#include <vector>

#include "brave/components/brave_rewards/core/ledger.h"

namespace brave_rewards::core {
class LedgerImpl;

namespace contribution {

class ContributionAC {
 public:
  explicit ContributionAC(LedgerImpl* ledger);

  ~ContributionAC();

  void Process(const uint64_t reconcile_stamp);

 private:
  void PreparePublisherList(std::vector<mojom::PublisherInfoPtr> list);

  void QueueSaved(const mojom::Result result);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace contribution
}  // namespace brave_rewards::core
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_AC_H_
