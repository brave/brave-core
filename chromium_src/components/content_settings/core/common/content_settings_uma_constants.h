/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_UMA_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_UMA_CONSTANTS_H_

#include "components/content_settings/core/common/content_settings_types.h"

namespace content_settings {

// Leave a gap between Chromium values and our values in the
// kContentSettingsTypeToHistogramValue array so that we don't have to renumber
// when new content settings types are added upstream.
//
// Do not change the value arbitrarily. This is used to validate we have a gap
// between Chromium's and Brave's histograms. This value must be less than 1000
// as upstream performs a sanity check that the total number of buckets isn't
// unreasonably large.
inline constexpr int kBraveValuesStart = 900;

inline constexpr int brave_value(int incr) {
  return kBraveValuesStart + incr;
}

static_assert(static_cast<int>(ContentSettingsType::kMaxValue) <
                  kBraveValuesStart,
              "There must a gap between the histograms used by Chromium, and "
              "the ones used by Brave.");

}  // namespace content_settings

#include <components/content_settings/core/common/content_settings_uma_constants.h>  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_UMA_CONSTANTS_H_
