/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/flags/did_override_variations_command_line_switches/did_override_variations_command_line_switches_parser_util.h"

#include <string>

#include "base/base_switches.h"
#include "base/check.h"
#include "base/command_line.h"
#include "components/variations/variations_switches.h"

namespace ads {

namespace {

bool HasSwitchWithValue(const std::string& switch_key) {
  CHECK(base::CommandLine::InitializedForCurrentProcess());
  const base::CommandLine* const command_line =
      base::CommandLine::ForCurrentProcess();
  return command_line->HasSwitch(switch_key) &&
         !command_line->GetSwitchValueASCII(switch_key).empty();
}

}  // namespace

bool ParseVariationsCommandLineSwitches() {
  if (HasSwitchWithValue(variations::switches::kFakeVariationsChannel) ||
      HasSwitchWithValue(variations::switches::kVariationsOverrideCountry)) {
    return true;
  } else {
    const char* kCommandLineSwitches[] = {
        switches::kEnableFeatures,
        variations::switches::kForceFieldTrialParams};

    CHECK(base::CommandLine::InitializedForCurrentProcess());
    const base::CommandLine* const command_line =
        base::CommandLine::ForCurrentProcess();

    std::string concatenated_command_line_switches;
    for (const char* command_line_switch : kCommandLineSwitches) {
      if (command_line->HasSwitch(command_line_switch)) {
        concatenated_command_line_switches +=
            command_line->GetSwitchValueASCII(command_line_switch);
      }
    }

    constexpr const char* kFeatureNames[] = {
        "AdRewards",        "AdServing",        "AntiTargeting",
        "Conversions",      "EligibleAds",      "EpsilonGreedyBandit",
        "FrequencyCapping", "InlineContentAds", "NewTabPageAds",
        "PermissionRules",  "PurchaseIntent",   "TextClassification",
        "UserActivity"};

    for (const char* feature_name : kFeatureNames) {
      if (concatenated_command_line_switches.find(feature_name) !=
          std::string::npos) {
        return true;
      }
    }
  }

  return false;
}

}  // namespace ads
