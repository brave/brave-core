/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/feature_override.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"

namespace base {
namespace internal {
namespace {

using DefaultStateOverrides =
    flat_map<std::reference_wrapper<const Feature>, FeatureState>;

using UnsortedDefaultStateOverrides =
    std::vector<DefaultStateOverrides::value_type>;

constexpr size_t kDefaultStateOverridesReserve = 64 * 4;

UnsortedDefaultStateOverrides& GetUnsortedDefaultStateOverrides() {
  static NoDestructor<UnsortedDefaultStateOverrides>
      startup_default_state_overrides([] {
        UnsortedDefaultStateOverrides v;
        v.reserve(kDefaultStateOverridesReserve);
        return v;
      }());
  return *startup_default_state_overrides;
}

const DefaultStateOverrides& GetDefaultStateOverrides() {
  static const NoDestructor<DefaultStateOverrides> default_state_overrides([] {
    DefaultStateOverrides sorted_overrides =
        std::move(GetUnsortedDefaultStateOverrides());
    DCHECK_EQ(GetUnsortedDefaultStateOverrides().capacity(), 0u);
    DLOG_IF(ERROR, sorted_overrides.size() > kDefaultStateOverridesReserve)
        << "Please increase kDefaultStateOverridesReserve. Feature overrides "
           "count: "
        << sorted_overrides.size()
        << ", reserve size: " << kDefaultStateOverridesReserve;
    return sorted_overrides;
  }());
  return *default_state_overrides;
}

}  // namespace

FeatureDefaultStateOverrider::FeatureDefaultStateOverrider(
    std::initializer_list<FeatureOverrideInfo> overrides) {
  auto& default_state_overrides = GetUnsortedDefaultStateOverrides();
#if DCHECK_IS_ON()
  {
    flat_set<std::reference_wrapper<const Feature>> new_overrides;
    new_overrides.reserve(overrides.size());
    for (const auto& override : overrides) {
      DCHECK(new_overrides.insert(override.first).second)
          << "Feature " << override.first.get().name
          << " is duplicated in the current override macros";
      DCHECK(!ranges::any_of(default_state_overrides,
                             [&override](const auto& v) {
                               return &v.first.get() == &override.first.get();
                             }))
          << "Feature " << override.first.get().name
          << " has already been overridden";
    }
  }
#endif
  default_state_overrides.insert(default_state_overrides.end(),
                                 overrides.begin(), overrides.end());
}

}  // namespace internal

// Custom comparator to use std::reference_wrapper as a key in a map/set.
static inline bool operator<(const std::reference_wrapper<const Feature>& lhs,
                             const std::reference_wrapper<const Feature>& rhs) {
  // Compare internal pointers directly, because there must only ever be one
  // struct instance for a given feature name.
  return &lhs.get() < &rhs.get();
}

bool FeatureList::IsFeatureOverridden(const std::string& feature_name) const {
  if (FeatureList::IsFeatureOverridden_ChromiumImpl(feature_name)) {
    return true;
  }

  const auto& default_state_overrides = internal::GetDefaultStateOverrides();
  for (const auto& default_state_override : default_state_overrides) {
    const Feature& feature = default_state_override.first.get();
    if (feature.name == feature_name) {
      return feature.default_state != default_state_override.second;
    }
  }
  return false;
}

// static
FeatureState FeatureList::GetCompileTimeFeatureState(const Feature& feature) {
  const auto& default_state_overrides = internal::GetDefaultStateOverrides();
  const auto default_state_override_it = default_state_overrides.find(feature);
  return default_state_override_it != default_state_overrides.end()
             ? default_state_override_it->second
             : feature.default_state;
}

}  // namespace base

// This replaces |default_state| compare blocks with a modified one that
// includes the compile time override check.
#define default_state name&& GetCompileTimeFeatureState(feature)
#define IsFeatureOverridden IsFeatureOverridden_ChromiumImpl

#include "src/base/feature_list.cc"

#undef IsFeatureOverridden
#undef default_state
