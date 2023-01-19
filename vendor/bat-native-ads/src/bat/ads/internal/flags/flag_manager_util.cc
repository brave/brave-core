/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/flags/flag_manager_util.h"

#include "bat/ads/internal/flags/environment/environment_types.h"
#include "bat/ads/internal/flags/flag_manager.h"

namespace ads {

bool IsProductionEnvironment() {
  return FlagManager::GetInstance()->GetEnvironmentType() ==
         EnvironmentType::kProduction;
}

}  // namespace ads
