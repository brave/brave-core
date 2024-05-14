/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/dynamic/confirmation_dynamic_user_data_builder.h"

#include "brave/components/brave_ads/core/internal/account/user_data/dynamic/diagnostic_id_user_data.h"
#include "brave/components/brave_ads/core/internal/account/user_data/dynamic/system_timestamp_user_data.h"

namespace brave_ads {

base::Value::Dict BuildDynamicUserData() {
  base::Value::Dict user_data;

  user_data.Merge(BuildDiagnosticIdUserData());
  user_data.Merge(BuildSystemTimestampUserData());

  return user_data;
}

}  // namespace brave_ads
