/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_FEATURE_OVERRIDE_H_
#define BRAVE_CHROMIUM_SRC_BASE_FEATURE_OVERRIDE_H_

#include <functional>
#include <initializer_list>
#include <utility>

#include "base/feature_list.h"

// Helpers to override base::Feature::default_state without patches.
//
// Usage:
//   1. Create chromium_src/.../features.cc override for a file that contains
//      features to override.
//   2. #include "base/feature_override.h"
//   3. Use OVERRIDE_FEATURE_DEFAULT_STATES macro:
//
//      OVERRIDE_FEATURE_DEFAULT_STATES({{
//          {kUpstreamFeature, base::FEATURE_ENABLED_BY_DEFAULT},
//      #if BUILDFLAG(IS_ANDROID)
//          {kAnotherUpstreamFeature, base::FEATURE_DISABLED_BY_DEFAULT},
//      #endif
//      }});

namespace base {
namespace internal {

// Perform base::Feature duplicates check and fills overriden states into a
// map that is used at runtime to get an override if available.
class BASE_EXPORT FeatureDefaultStateOverrider {
 public:
  using FeatureOverrideInfo =
      std::pair<std::reference_wrapper<const Feature>, FeatureState>;

  FeatureDefaultStateOverrider(
      std::initializer_list<FeatureOverrideInfo> overrides);
};

}  // namespace internal
}  // namespace base

// Feature override uses global constructors, we disable `global-constructors`
// warning inside this macro to instantiate the overrider without warnings.
// clang-format off
#define OVERRIDE_FEATURE_DEFAULT_STATES(...)                    \
  _Pragma("clang diagnostic push")                              \
  _Pragma("clang diagnostic ignored \"-Wglobal-constructors\"") \
  static const ::base::internal::FeatureDefaultStateOverrider   \
      g_feature_default_state_overrider __VA_ARGS__;            \
  _Pragma("clang diagnostic pop")                               \
  static_assert(true, "") /* for a semicolon requirement */
// clang-format on

#endif  // BRAVE_CHROMIUM_SRC_BASE_FEATURE_OVERRIDE_H_
