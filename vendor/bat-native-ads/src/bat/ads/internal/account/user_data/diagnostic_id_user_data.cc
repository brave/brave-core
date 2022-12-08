/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/diagnostic_id_user_data.h"

#include <string>

#include "base/guid.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "brave/components/brave_ads/common/pref_names.h"

namespace ads::user_data {

namespace {
constexpr char kDiagnosticIdKey[] = "diagnosticId";
}  // namespace

base::Value::Dict GetDiagnosticId() {
  base::Value::Dict user_data;

  const std::string diagnostic_id =
      AdsClientHelper::GetInstance()->GetStringPref(prefs::kDiagnosticId);

  const base::GUID guid = base::GUID::ParseCaseInsensitive(diagnostic_id);
  if (guid.is_valid()) {
    user_data.Set(kDiagnosticIdKey, diagnostic_id);
  }

  return user_data;
}

}  // namespace ads::user_data
