/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_AC_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_AC_H_

#include <vector>

#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace brave_rewards::internal::contribution {

class ContributionAC {
 public:
  void Process(const uint64_t reconcile_stamp);

 private:
  void PreparePublisherList(std::vector<mojom::PublisherInfoPtr> list);

  void QueueSaved(const mojom::Result result);
};

}  // namespace brave_rewards::internal::contribution
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_AC_H_
