/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <array>
#include <string_view>

#include "base/feature.h"

namespace base {
namespace {

// Array of all compile overridden features, sorted alphabetically.
constexpr std::string_view kCompileOverriddenFeatures[] = {
#define OVERRIDDEN_FEATURE(name) name,
#include "brave/base/compile_overridden_features.inc"
#undef OVERRIDDEN_FEATURE
};

static_assert(std::ranges::is_sorted(kCompileOverriddenFeatures),
              "kCompileOverriddenFeatures must stay sorted");

}  // namespace

namespace internal {

#if !defined(OFFICIAL_BUILD)
// Defines the symbol every BASE_OVERRIDDEN_FEATURE refers to. Only the symbol
// matters, never its value, so it is left value-initialized. BASE_EXPORT is
// repeated here because Clang takes a specialization's visibility from its
// definition rather than from the declaration in the primary template.
#define OVERRIDDEN_FEATURE(name) \
  template <>                    \
  BASE_EXPORT const bool         \
      CompileOverriddenFeature<std::to_array(name)>::kIsListed = {};
#include "brave/base/compile_overridden_features.inc"
#undef OVERRIDDEN_FEATURE
#endif  // !defined(OFFICIAL_BUILD)

}  // namespace internal

bool IsCompileOverriddenFeature(std::string_view feature_name) {
  return std::ranges::binary_search(kCompileOverriddenFeatures, feature_name);
}

}  // namespace base
