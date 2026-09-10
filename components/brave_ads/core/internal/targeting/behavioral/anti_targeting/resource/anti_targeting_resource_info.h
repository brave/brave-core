/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_ANTI_TARGETING_RESOURCE_ANTI_TARGETING_RESOURCE_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_ANTI_TARGETING_RESOURCE_ANTI_TARGETING_RESOURCE_INFO_H_

#include <optional>
#include <string>

#include "base/values.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

class GURL;

namespace brave_ads {

using AntiTargetingSiteList = absl::flat_hash_set<GURL>;
using AntiTargetingCreativeSetMap =
    absl::flat_hash_map</*creative_set_id*/ std::string, AntiTargetingSiteList>;

struct AntiTargetingResourceInfo final {
  AntiTargetingResourceInfo();

  AntiTargetingResourceInfo(const AntiTargetingResourceInfo&) = delete;
  AntiTargetingResourceInfo& operator=(const AntiTargetingResourceInfo&) =
      delete;

  AntiTargetingResourceInfo(AntiTargetingResourceInfo&&) noexcept;
  AntiTargetingResourceInfo& operator=(AntiTargetingResourceInfo&&) noexcept;

  ~AntiTargetingResourceInfo();

  static std::optional<AntiTargetingResourceInfo> MaybeFromDict(
      base::DictValue dict);

  std::optional<int> version;
  AntiTargetingCreativeSetMap creative_sets;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_ANTI_TARGETING_RESOURCE_ANTI_TARGETING_RESOURCE_INFO_H_
