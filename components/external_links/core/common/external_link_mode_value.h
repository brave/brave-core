/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EXTERNAL_LINKS_CORE_COMMON_EXTERNAL_LINK_MODE_VALUE_H_
#define BRAVE_COMPONENTS_EXTERNAL_LINKS_CORE_COMMON_EXTERNAL_LINK_MODE_VALUE_H_

#include <optional>

#include "base/values.h"
#include "brave/components/external_links/core/common/external_link_mode.h"

namespace external_links {

// Non-permission website settings must be stored as dictionaries.
inline constexpr char kExternalLinkModeKey[] = "mode";

inline base::Value ExternalLinkModeToValue(ExternalLinkMode mode) {
  return base::Value(
      base::DictValue().Set(kExternalLinkModeKey, static_cast<int>(mode)));
}

inline std::optional<ExternalLinkMode> ExternalLinkModeFromValue(
    const base::Value& value) {
  const auto* dict = value.GetIfDict();
  if (!dict) {
    return std::nullopt;
  }
  std::optional<int> mode = dict->FindInt(kExternalLinkModeKey);
  if (!mode || *mode < 0 ||
      *mode > static_cast<int>(ExternalLinkMode::kMaxValue)) {
    return std::nullopt;
  }
  return static_cast<ExternalLinkMode>(*mode);
}

}  // namespace external_links

#endif  // BRAVE_COMPONENTS_EXTERNAL_LINKS_CORE_COMMON_EXTERNAL_LINK_MODE_VALUE_H_
