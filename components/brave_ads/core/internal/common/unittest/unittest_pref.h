/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PREF_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PREF_H_

#include <string>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_info.h"

namespace brave_ads {

using PrefMap = base::flat_map</*uuid=*/std::string, PrefInfo>;
PrefMap& Prefs();

PrefInfo& Pref(const std::string& path);

std::string GetPrefValue(const std::string& path);
void SetPrefValue(const std::string& path, const std::string& value);

std::string GetDefaultPrefValue(const std::string& path);
void SetDefaultPrefValue(const std::string& path, const std::string& value);

void ClearPrefValue(const std::string& path);

bool HasPrefPathValue(const std::string& path);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PREF_H_
