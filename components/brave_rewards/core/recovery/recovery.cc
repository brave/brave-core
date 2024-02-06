/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/recovery/recovery.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state.h"

namespace brave_rewards::internal {
namespace recovery {

Recovery::Recovery(RewardsEngineImpl& engine)
    : engine_(engine), empty_balance_(engine) {}

Recovery::~Recovery() = default;

void Recovery::Check() {
  if (!engine_->state()->GetEmptyBalanceChecked()) {
    engine_->Log(FROM_HERE) << "Running empty balance check...";
    empty_balance_.Check();
  }
}

}  // namespace recovery
}  // namespace brave_rewards::internal
