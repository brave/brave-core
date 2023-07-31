/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/components/browsing_data/core/browsing_data_utils.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#define GetDeletionPreferenceFromDataType \
  GetDeletionPreferenceFromDataType_ChromiumImpl
#define GetDataTypeFromDeletionPreference \
  GetDataTypeFromDeletionPreference_ChromiumImpl
#define NUM_TYPES \
  NUM_TYPES:      \
  case BrowsingDataType::BRAVE_AI_CHAT
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
#include "src/components/browsing_data/core/browsing_data_utils.cc"
#if BUILDFLAG(ENABLE_AI_CHAT)
#undef NUM_TYPES
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
#undef GetDataTypeFromDeletionPreference
#undef GetDeletionPreferenceFromDataType

namespace browsing_data {

#if BUILDFLAG(ENABLE_AI_CHAT)
bool GetDeletionPreferenceFromDataType(
    BrowsingDataType data_type,
    ClearBrowsingDataTab clear_browsing_data_tab,
    std::string* out_pref) {
  if (clear_browsing_data_tab == ClearBrowsingDataTab::ADVANCED &&
      data_type == BrowsingDataType::BRAVE_AI_CHAT) {
    *out_pref = prefs::kDeleteBraveLeoHistory;
    return true;
  }

  return GetDeletionPreferenceFromDataType_ChromiumImpl(
      data_type, clear_browsing_data_tab, out_pref);
}

absl::optional<BrowsingDataType> GetDataTypeFromDeletionPreference(
    const std::string& pref_name) {
  using DataTypeMap = base::flat_map<std::string, BrowsingDataType>;
  static base::NoDestructor<DataTypeMap> preference_to_datatype(
      std::initializer_list<DataTypeMap::value_type>{
          {prefs::kDeleteBraveLeoHistory, BrowsingDataType::BRAVE_AI_CHAT},
      });

  auto iter = preference_to_datatype->find(pref_name);
  if (iter != preference_to_datatype->end()) {
    return iter->second;
  }
  return GetDataTypeFromDeletionPreference_ChromiumImpl(pref_name);
}
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

}  // namespace browsing_data
