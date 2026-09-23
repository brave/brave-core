/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_FEATURE_INTERNAL_H_
#define BRAVE_CHROMIUM_SRC_BASE_FEATURE_INTERNAL_H_

#include <base/feature_internal.h>  // IWYU pragma: export

#include <string_view>

#include "base/containers/fixed_flat_set.h"

namespace base::internal {

// A set of all compile overridden features.
inline constexpr auto kCompileOverriddenFeatures =
    base::MakeFixedFlatSet<std::string_view>(base::sorted_unique,
                                             {
// CHROMIUM_SRC_NOLINT
#define X(name) name,
#include "brave/base/compile_overridden_features.inc"
#undef X
                                             });

constexpr bool IsCompileOverriddenFeature(std::string_view feature_name) {
  return kCompileOverriddenFeatures.contains(feature_name);
}

}  // namespace base::internal

// CHROMIUM_SRC_NOLINT
#define BASE_OVERRIDDEN_FEATURE_INTERNAL_IS_LISTED_CHECK(name) \
  static_assert(                                               \
      ::base::internal::IsCompileOverriddenFeature(name),      \
      "Feature is not listed in brave/base/compile_overridden_features.inc")

// CHROMIUM_SRC_NOLINT
#define BASE_OVERRIDDEN_FEATURE_INTERNAL_3_ARGS(feature, name, default_state) \
  BASE_OVERRIDDEN_FEATURE_INTERNAL_IS_LISTED_CHECK(name);                     \
  BASE_FEATURE(feature, name, default_state)

// CHROMIUM_SRC_NOLINT
#define BASE_OVERRIDDEN_FEATURE_INTERNAL_2_ARGS(feature, default_state) \
  BASE_OVERRIDDEN_FEATURE_INTERNAL_IS_LISTED_CHECK(                     \
      std::string_view(#feature).substr(1));                            \
  BASE_FEATURE(feature, default_state)

// Same as BASE_FEATURE, but marks the feature as having a default state that
// Brave overrides at compile time. Such features are reported as overridden by
// FeatureList::IsFeatureOverridden and FeatureList::GetStateIfOverridden, and
// must be listed in brave/base/compile_overridden_features.inc.
// CHROMIUM_SRC_NOLINT
#define BASE_OVERRIDDEN_FEATURE(...)                        \
  BASE_FEATURE_INTERNAL_GET_FEATURE_MACRO(                  \
      __VA_ARGS__, BASE_OVERRIDDEN_FEATURE_INTERNAL_3_ARGS, \
      BASE_OVERRIDDEN_FEATURE_INTERNAL_2_ARGS)              \
  (__VA_ARGS__)

#endif  // BRAVE_CHROMIUM_SRC_BASE_FEATURE_INTERNAL_H_
