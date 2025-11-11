/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/browsing_data/core/browsing_data_utils.h"

#include <optional>
#include <string_view>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"

#define GetDeletionPreferenceFromDataType \
  GetDeletionPreferenceFromDataType_ChromiumImpl
#define GetDataTypeFromDeletionPreference \
  GetDataTypeFromDeletionPreference_ChromiumImpl
#if BUILDFLAG(ENABLE_AI_CHAT)
#define TABS \
  TABS:      \
  case BrowsingDataType::BRAVE_AI_CHAT
#endif

#include <components/browsing_data/core/browsing_data_utils.cc>

#if BUILDFLAG(ENABLE_AI_CHAT)
#undef TABS
#endif
#undef GetDataTypeFromDeletionPreference
#undef GetDeletionPreferenceFromDataType

namespace browsing_data {

bool GetDeletionPreferenceFromDataType(
    BrowsingDataType data_type,
    ClearBrowsingDataTab clear_browsing_data_tab,
    std::string* out_pref) {
#if BUILDFLAG(ENABLE_AI_CHAT)
  if (clear_browsing_data_tab == ClearBrowsingDataTab::ADVANCED &&
      data_type == BrowsingDataType::BRAVE_AI_CHAT) {
    *out_pref = prefs::kDeleteBraveLeoHistory;
    return true;
  }
#endif

  return GetDeletionPreferenceFromDataType_ChromiumImpl(
      data_type, clear_browsing_data_tab, out_pref);
}

std::optional<BrowsingDataType> GetDataTypeFromDeletionPreference(
    const std::string& pref_name) {
#if BUILDFLAG(ENABLE_AI_CHAT)
  static constexpr auto kPreferenceToDataType =
      base::MakeFixedFlatMap<std::string_view, BrowsingDataType>({
          {prefs::kDeleteBraveLeoHistory, BrowsingDataType::BRAVE_AI_CHAT},
          {prefs::kDeleteBraveLeoHistoryOnExit,
           BrowsingDataType::BRAVE_AI_CHAT},
      });

  if (const auto* data_type =
          base::FindOrNull(kPreferenceToDataType, pref_name)) {
    return *data_type;
  }
#endif
  return GetDataTypeFromDeletionPreference_ChromiumImpl(pref_name);
}

}  // namespace browsing_data
