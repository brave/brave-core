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
#include "components/browsing_data/core/pref_names.h"

namespace {

#if BUILDFLAG(ENABLE_AI_CHAT)
std::optional<browsing_data::BrowsingDataType>
GetAIChatDataTypeFromDeletionPreference(const std::string& pref_name) {
  static constexpr auto kPreferenceToDataType =
      base::MakeFixedFlatMap<std::string_view, browsing_data::BrowsingDataType>(
          {{browsing_data::prefs::kDeleteBraveLeoHistory,
            browsing_data::BrowsingDataType::BRAVE_AI_CHAT},
           {browsing_data::prefs::kDeleteBraveLeoHistoryOnExit,
            browsing_data::BrowsingDataType::BRAVE_AI_CHAT}});
  if (const auto* data_type =
          base::FindOrNull(kPreferenceToDataType, pref_name)) {
    return *data_type;
  }
  return std::nullopt;
}
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

}  // namespace

#include <components/browsing_data/core/browsing_data_utils.cc>
