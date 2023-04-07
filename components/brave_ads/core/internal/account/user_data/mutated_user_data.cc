/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/mutated_user_data.h"

#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"

namespace brave_ads::user_data {

namespace {
constexpr char kMutatedKey[] = "mutated";
}  // namespace

base::Value::Dict GetMutated() {
  base::Value::Dict user_data;

  if (ConfirmationStateManager::GetInstance()->is_mutated() ||
      ClientStateManager::GetInstance()->is_mutated()) {
    user_data.Set(kMutatedKey, true);
  }

  return user_data;
}

}  // namespace brave_ads::user_data
