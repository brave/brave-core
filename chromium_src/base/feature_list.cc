/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "base/feature_override.h"
#include "base/no_destructor.h"

namespace base {
namespace {

using FeatureDefaultStateOverrides =
    base::flat_map<const Feature*, FeatureState>;

FeatureDefaultStateOverrides& GetFeatureDefaultStateOverrides() {
  static base::NoDestructor<FeatureDefaultStateOverrides> overrides;
  return *overrides;
}

FeatureState GetDefaultOrOverriddenState(const Feature& feature) {
  const auto& overrides = GetFeatureDefaultStateOverrides();
  auto override_it = overrides.find(&feature);
  return override_it == overrides.end() ? feature.default_state
                                        : override_it->second;
}

}  // namespace

namespace internal {

FeatureDefaultStateOverride::FeatureDefaultStateOverride(
    const base::Feature& feature,
    FeatureState default_state) {
  const auto& inserted =
      GetFeatureDefaultStateOverrides().emplace(&feature, default_state);
  DCHECK(inserted.second) << "Feature " << feature.name
                          << "has already been overridden";
}

}  // namespace internal
}  // namespace base

#define default_state name&& GetDefaultOrOverriddenState(feature)

#include "../../../base/feature_list.cc"

#undef default_state
