/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_RECOVERY_RECOVERY_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_RECOVERY_RECOVERY_H_

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/recovery/recovery_empty_balance.h"

namespace ledger {
class LedgerImpl;

namespace recovery {

class Recovery {
 public:
  explicit Recovery(LedgerImpl& ledger);
  ~Recovery();

  void Check();

 private:
  const raw_ref<LedgerImpl> ledger_;
  EmptyBalance empty_balance_;
};

}  // namespace recovery
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_RECOVERY_RECOVERY_H_
