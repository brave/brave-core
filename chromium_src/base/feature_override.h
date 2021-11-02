/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_FEATURE_OVERRIDE_H_
#define BRAVE_CHROMIUM_SRC_BASE_FEATURE_OVERRIDE_H_

#include "base/feature_list.h"

// Helpers to override base::Feature::default_state without patching.
// Usage:
// 1. Create chromium_src/.../features.cc override for a file that contains the
//    feature to override.
// 2. #include "base/feature_override.h"
// 3. Declare ENABLE_FEATURE_BY_DEFAULT(...) or DISABLE_FEATURE_BY_DEFAULT(...).

namespace base {

class BASE_EXPORT FeatureDefaultStateOverride {
 public:
  constexpr FeatureDefaultStateOverride(const base::Feature& feature,
                                        FeatureState default_state) {
    feature.mutable_default_state = default_state;
  }
};

}  // namespace base

// clang-format off
#define OVERRIDE_FEATURE_DEFAULT_STATE_UNIQUE(feature, default_state, key) \
  _Pragma("clang diagnostic push")                                         \
  _Pragma("clang diagnostic ignored \"-Wglobal-constructors\"")            \
  static const ::base::FeatureDefaultStateOverride                         \
      g_feature_default_state_override_##key(feature, default_state);      \
  _Pragma("clang diagnostic pop")                                          \
  static_assert(true, "") /* for a semicolon requirement */
// clang-format on

#define OVERRIDE_FEATURE_DEFAULT_STATE_EXPANDER(feature, default_state, key) \
  OVERRIDE_FEATURE_DEFAULT_STATE_UNIQUE(feature, default_state, key)

#define OVERRIDE_FEATURE_DEFAULT_STATE(feature, default_state) \
  OVERRIDE_FEATURE_DEFAULT_STATE_EXPANDER(feature, default_state, __COUNTER__)

#define ENABLE_FEATURE_BY_DEFAULT(feature) \
  OVERRIDE_FEATURE_DEFAULT_STATE(feature, ::base::FEATURE_ENABLED_BY_DEFAULT)

#define DISABLE_FEATURE_BY_DEFAULT(feature) \
  OVERRIDE_FEATURE_DEFAULT_STATE(feature, ::base::FEATURE_DISABLED_BY_DEFAULT)

#endif  // BRAVE_CHROMIUM_SRC_BASE_FEATURE_OVERRIDE_H_
