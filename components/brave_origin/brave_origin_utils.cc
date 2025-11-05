/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/brave_origin_utils.h"

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_origin/brave_origin_policy_info.h"
#include "brave/components/brave_origin/features.h"

namespace brave_origin {

bool IsBraveOriginEnabled() {
  // TODO(https://github.com/brave/brave-browser/issues/47463)
  // Get the actual purchase state from SKU service.
  return base::FeatureList::IsEnabled(features::kBraveOrigin);
}

std::string GetBraveOriginPrefKey(std::string_view policy_key,
                                  std::optional<std::string_view> profile_id) {
  if (!profile_id.has_value()) {
    return std::string(policy_key);
  }

  // For profile prefs, use profile_id.policy_key format
  CHECK(!profile_id.value().empty());
  return base::StrCat({profile_id.value(), ".", policy_key});
}

}  // namespace brave_origin
