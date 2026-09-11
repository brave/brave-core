/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_FEATURE_H_
#define BRAVE_CHROMIUM_SRC_BASE_FEATURE_H_

#include <base/feature.h>  // IWYU pragma: export

#include <array>
#include <cstddef>
#include <string_view>

#include "base/base_export.h"

namespace base {

// True for features defined with BASE_OVERRIDDEN_FEATURE. The names live in
// brave/base/compile_overridden_features.cc, so editing that list does not
// rebuild every translation unit including this header. FeatureList uses this
// to report compile-time overrides as overridden.
BASE_EXPORT bool IsCompileOverriddenFeature(std::string_view feature_name);

namespace internal {

#if !defined(OFFICIAL_BUILD)
// Drops the leading `k` of a feature's C++ identifier. std::array is the
// carrier because std::string_view is not a structural type and so cannot be
// a non-type template argument.
template <size_t N>
consteval std::array<char, N - 1> FeatureNameFromIdentifier(
    const char (&identifier)[N]) {
  std::array<char, N - 1> name = {};
  for (size_t i = 1; i < N; ++i) {
    name[i - 1] = identifier[i];
  }
  return name;
}

// `kIsListed` is defined, in compile_overridden_features.cc, only for the
// names listed there, so a BASE_OVERRIDDEN_FEATURE missing from that list
// fails to link instead of being checked against a constexpr list compiled
// into every translation unit.
template <std::array kName>
struct CompileOverriddenFeature {
  BASE_EXPORT static const bool kIsListed;
};
#endif  // !defined(OFFICIAL_BUILD)

}  // namespace internal
}  // namespace base

// The pointer is never read; `used` is what keeps it, and with it the
// reference to `kIsListed`, through the compiler's unused-variable elision and
// the linker's dead stripping, so that a name missing from the list surfaces
// as an undefined symbol naming the feature.
#if !defined(OFFICIAL_BUILD)
// CHROMIUM_SRC_NOLINT
#define BASE_OVERRIDDEN_FEATURE_INTERNAL_IS_LISTED_CHECK(feature, name) \
  __attribute__((used)) static const bool* feature##_IsListedCheck() {  \
    return &base::internal::CompileOverriddenFeature<name>::kIsListed;  \
  }                                                                     \
  static_assert(true, "")  // Force a semicolon after the macro.
#else
// CHROMIUM_SRC_NOLINT
#define BASE_OVERRIDDEN_FEATURE_INTERNAL_IS_LISTED_CHECK(feature, name) \
  static_assert(true, "")  // Force a semicolon after the macro.
#endif

// CHROMIUM_SRC_NOLINT
#define BASE_OVERRIDDEN_FEATURE_INTERNAL_3_ARGS(feature, name, default_state) \
  BASE_OVERRIDDEN_FEATURE_INTERNAL_IS_LISTED_CHECK(feature,                   \
                                                   std::to_array(name));      \
  BASE_FEATURE(feature, name, default_state)

// CHROMIUM_SRC_NOLINT
#define BASE_OVERRIDDEN_FEATURE_INTERNAL_2_ARGS(feature, default_state) \
  BASE_OVERRIDDEN_FEATURE_INTERNAL_IS_LISTED_CHECK(                     \
      feature, base::internal::FeatureNameFromIdentifier(#feature));    \
  BASE_FEATURE(feature, default_state)

// Same as BASE_FEATURE, but marks the feature as having a default state that
// Brave overrides at compile time. Such features are reported as overridden by
// FeatureList::IsFeatureOverridden and FeatureList::GetStateIfOverridden, and
// must be listed in brave/base/compile_overridden_features.cc.
// CHROMIUM_SRC_NOLINT
#define BASE_OVERRIDDEN_FEATURE(...)                        \
  BASE_FEATURE_INTERNAL_GET_FEATURE_MACRO(                  \
      __VA_ARGS__, BASE_OVERRIDDEN_FEATURE_INTERNAL_3_ARGS, \
      BASE_OVERRIDDEN_FEATURE_INTERNAL_2_ARGS)              \
  (__VA_ARGS__)

#endif  // BRAVE_CHROMIUM_SRC_BASE_FEATURE_H_
