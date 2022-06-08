/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sync/base/model_type.h"
#include "components/sync/base/user_selectable_type.h"

#define kWifiConfigurations kVgBodies:     \
  return prefs::kSyncVgBodies;             \
case UserSelectableType::kVgSpendStatuses: \
  return prefs::kSyncVgSpendStatuses;      \
case UserSelectableType::kWifiConfigurations
#include "src/components/sync/base/sync_prefs.cc"
#undef kWifiConfigurations
