/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_CONDITION_MATCHER_DIAGNOSTICS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_CONDITION_MATCHER_DIAGNOSTICS_UTIL_H_

#include <optional>
#include <string>

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager.h"
#include "brave/components/brave_ads/core/internal/segments/segment_types.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace base {
class DictValue;
}  // namespace base

namespace brave_ads {

void GetConditionMatchersCallback(
    GetConditionMatchersDiagnosticsCallback callback,
    bool success,
    const SegmentList& segments,
    CreativeNewTabPageAdList creative_ads);

void BuildTestDiagnosticsConditionMatcherResultCallback(
    TestDiagnosticsConditionMatcherCallback callback,
    std::string pref_path,
    std::string condition,
    std::optional<std::string> test_value,
    base::DictValue virtual_prefs);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_MANAGER_CONDITION_MATCHER_DIAGNOSTICS_UTIL_H_
