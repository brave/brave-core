/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/flags_ui/flags_state.h"

#include <string_view>

#include "base/feature_override.h"
#include "base/strings/strcat.h"

#include "src/components/flags_ui/flags_state.cc"

namespace flags_ui {
namespace {

void AppendCurrentFeatureStateIfDefault(
    const FeatureEntry& entry,
    const std::set<std::string>& enabled_entries,
    base::Value::List& result) {
  if (base::ranges::any_of(result, [](const base::Value& v) {
        return *v.GetDict().FindBool("selected");
      })) {
    // A non-Default state is selected on Flags UI. In this case we don't know
    // the actual default state we will get on restart, because it may be
    // overridden by variations or a command line.
    //
    // We can show hardcoded default state, but this may lead to a confusion if
    // there's a study that overrides it. A sane approach here is to not display
    // the default state if the state is manually changed via Flags UI.
    return;
  }

  DCHECK(entry.feature.feature);
  const auto& feature = *entry.feature.feature;
  const bool is_feature_enabled_now = base::FeatureList::IsEnabled(feature);
  const std::string_view current_state = is_feature_enabled_now
                                             ? kGenericExperimentChoiceEnabled
                                             : kGenericExperimentChoiceDisabled;

  const bool is_feature_enabled_by_default =
      base::GetCompileTimeFeatureState(feature) ==
      base::FeatureState::FEATURE_ENABLED_BY_DEFAULT;
  const std::string_view current_state_flag =
      is_feature_enabled_now != is_feature_enabled_by_default ? "*" : "";

  // Add current state to "Default" selector and append "*" if the state differs
  // from the hardcoded default (overridden by variations, a command line or
  // something else).
  auto* description = result.front().GetDict().FindString("description");
  DCHECK(description);
  DCHECK_EQ(*description, kGenericExperimentChoiceDefault);
  base::StrAppend(description, {" (", current_state, current_state_flag, ")"});
}

}  // namespace

// Returns the Value::List representing the choice data in the specified entry.
base::Value::List FlagsState::CreateOptionsData(
    const FeatureEntry& entry,
    const std::set<std::string>& enabled_entries) const {
  base::Value::List result =
      ::flags_ui::CreateOptionsData(entry, enabled_entries);

  if (entry.type == FeatureEntry::FEATURE_VALUE ||
      entry.type == FeatureEntry::FEATURE_WITH_PARAMS_VALUE) {
    AppendCurrentFeatureStateIfDefault(entry, enabled_entries, result);
  }

  return result;
}

}  // namespace flags_ui
