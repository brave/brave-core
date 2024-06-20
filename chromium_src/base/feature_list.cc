/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"

#include <optional>
#include <string_view>
#include <utility>

#include "base/check_op.h"
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
    std::vector<std::pair<std::string_view, FeatureState>>;

constexpr size_t kDefaultStateOverridesReserve = 64 * 6;

DefaultStateOverrides& GetMutableDefaultStateOverrides() {
  static NoDestructor<DefaultStateOverrides> default_state_overrides([] {
    DefaultStateOverrides v;
    v.reserve(kDefaultStateOverridesReserve);
    return v;
  }());
  return *default_state_overrides;
}

const DefaultStateOverrides& GetDefaultStateOverrides() {
  auto& default_state_overrides = GetMutableDefaultStateOverrides();
  static const size_t kOverridesSortedSize =
      [](DefaultStateOverrides& default_state_overrides) {
        ranges::sort(default_state_overrides, {},
                     &DefaultStateOverrides::value_type::first);
        DCHECK_GE(kDefaultStateOverridesReserve, default_state_overrides.size())
            << "Please increase kDefaultStateOverridesReserve";
        return default_state_overrides.size();
      }(default_state_overrides);

  // After the sort the overrides should not be modified.
  CHECK_EQ(kOverridesSortedSize, default_state_overrides.size());
  return default_state_overrides;
}

}  // namespace

FeatureDefaultStateOverrider::FeatureDefaultStateOverrider(
    std::initializer_list<FeatureOverrideInfo> overrides) {
  auto& default_state_overrides = GetMutableDefaultStateOverrides();
#if DCHECK_IS_ON()
  flat_set<std::string_view> new_overrides;
  new_overrides.reserve(overrides.size());
#endif
  for (const auto& override : overrides) {
    const std::string_view feature_name = override.first.get().name;
#if DCHECK_IS_ON()
    DCHECK(!feature_name.empty());
    DCHECK(new_overrides.insert(feature_name).second)
        << "Feature " << feature_name
        << " is duplicated in the current override macros";
    DCHECK(!ranges::any_of(
        default_state_overrides,
        [&feature_name, override_state = override.second](const auto& v) {
          return v.first == feature_name && v.second != override_state;
        }))
        << "Feature " << feature_name
        << " has been overridden with different states";
#endif
    default_state_overrides.emplace_back(feature_name, override.second);
  }
}

}  // namespace internal

FeatureState GetCompileTimeFeatureState(const Feature& feature) {
  const auto& default_state_overrides = internal::GetDefaultStateOverrides();
  const auto default_state_override_it =
      ranges::lower_bound(default_state_overrides, feature.name, {},
                          &internal::DefaultStateOverrides::value_type::first);

  if (default_state_override_it != default_state_overrides.end() &&
      default_state_override_it->first == feature.name) {
    return default_state_override_it->second;
  }

  return feature.default_state;
}

}  // namespace base

// This replaces |default_state| compare blocks with a modified one that
// includes the compile time override check.
#define default_state name&& GetCompileTimeFeatureState(feature)

#define BRAVE_FEATURE_LIST_GET_OVERRIDE_ENTRY_BY_FEATURE_NAME           \
  using OverridesMap = decltype(overrides_);                            \
  static const NoDestructor<OverridesMap> kCompileTimeOverrides([]() {  \
    OverridesMap compile_time_overrides;                                \
    for (const auto& override : internal::GetDefaultStateOverrides()) { \
      compile_time_overrides.emplace(                                   \
          std::string(override.first),                                  \
          OverrideEntry(override.second == FEATURE_ENABLED_BY_DEFAULT   \
                            ? OverrideState::OVERRIDE_ENABLE_FEATURE    \
                            : OverrideState::OVERRIDE_DISABLE_FEATURE,  \
                        nullptr));                                      \
    }                                                                   \
    return compile_time_overrides;                                      \
  }());                                                                 \
  it = kCompileTimeOverrides->find(name);                               \
  if (it != kCompileTimeOverrides->end()) {                             \
    const OverrideEntry& entry = it->second;                            \
    return &entry;                                                      \
  }

#include "src/base/feature_list.cc"

#undef default_state
#undef BRAVE_FEATURE_LIST_GET_OVERRIDE_ENTRY_BY_FEATURE_NAME
