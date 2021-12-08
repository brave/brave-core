/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/feature_override.h"
#include "base/no_destructor.h"

namespace base {
namespace internal {
namespace {

using DefaultStateOverrides =
    base::flat_map<std::reference_wrapper<const Feature>, FeatureState>;

DefaultStateOverrides& GetDefaultStateOverrides() {
  static base::NoDestructor<DefaultStateOverrides> default_state_overrides;
  return *default_state_overrides;
}

inline FeatureState GetDefaultOrOverriddenFeatureState(const Feature& feature) {
  const auto& default_state_overrides = GetDefaultStateOverrides();
  const auto default_state_override_it = default_state_overrides.find(feature);
  return default_state_override_it != default_state_overrides.end()
             ? default_state_override_it->second
             : feature.default_state;
}

}  // namespace

FeatureDefaultStateOverrider::FeatureDefaultStateOverrider(
    std::initializer_list<FeatureOverrideInfo> overrides) {
  auto& default_state_overrides = GetDefaultStateOverrides();
#if DCHECK_IS_ON()
  {
    base::flat_set<std::reference_wrapper<const Feature>> new_overrides;
    new_overrides.reserve(overrides.size());
    for (const auto& override : overrides) {
      DCHECK(new_overrides.insert(override.first).second)
          << "Feature " << override.first.get().name
          << " is duplicated in the current override macros";
      DCHECK(!base::Contains(default_state_overrides, override.first))
          << "Feature " << override.first.get().name
          << " has already been overridden";
    }
  }
#endif
  default_state_overrides.insert(overrides.begin(), overrides.end());
}

}  // namespace internal

// Custom comparator to use std::reference_wrapper as a key in a map/set.
static inline bool operator<(const std::reference_wrapper<const Feature>& lhs,
                             const std::reference_wrapper<const Feature>& rhs) {
  // Compare internal pointers directly, because there must only ever be one
  // struct instance for a given feature name.
  return &lhs.get() < &rhs.get();
}

}  // namespace base

// This replaces |default_state| compare blocks with a modified one that
// includes the state override check.
#define default_state \
  name&& internal::GetDefaultOrOverriddenFeatureState(feature)

#include "src/base/feature_list.cc"

#undef default_state
