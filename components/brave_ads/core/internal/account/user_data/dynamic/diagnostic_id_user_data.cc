/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/dynamic/diagnostic_id_user_data.h"

#include <string>

#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

namespace {
constexpr char kDiagnosticIdKey[] = "diagnosticId";
}  // namespace

base::Value::Dict BuildDiagnosticIdUserData() {
  if (!UserHasJoinedBraveRewards()) {
    return {};
  }

  base::Value::Dict user_data;

  const std::string diagnostic_id = GetProfileStringPref(prefs::kDiagnosticId);

  const base::Uuid uuid = base::Uuid::ParseCaseInsensitive(diagnostic_id);
  if (uuid.is_valid()) {
    user_data.Set(kDiagnosticIdKey, diagnostic_id);
  }

  return user_data;
}

}  // namespace brave_ads
