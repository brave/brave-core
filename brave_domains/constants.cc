// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/brave_domains/constants.h"

#include <string>

#include "base/command_line.h"
#include "base/containers/fixed_flat_set.h"
#include "base/logging.h"

namespace brave_domains {

namespace {

constexpr char kGate3ProdURL[] = "https://gate3.wallet.brave.com";
constexpr char kGate3DevURL[] = "https://gate3.wallet.brave.software";
constexpr char kEnvGate3Switch[] = "env-gate3";

}  // namespace

bool IsValidSwitchValue(const std::string& value) {
  static constexpr auto kAllowedValues =
      base::MakeFixedFlatSet<std::string_view>(
          {kBraveServicesSwitchValueDev, kBraveServicesSwitchValueStaging,
           kBraveServicesSwitchValueProduction});
  return kAllowedValues.contains(value);
}

void MaybeWarnSwitchValue(const std::string& switch_name,
                          const std::string& value) {
  if (!value.empty()) {
    if (!IsValidSwitchValue(value)) {
      LOG(ERROR) << "Invalid value for --" << switch_name << ": \"" << value
                 << "\". Expected \"" << kBraveServicesSwitchValueDev
                 << "\", \"" << kBraveServicesSwitchValueStaging << "\", or \""
                 << kBraveServicesSwitchValueProduction << "\".";
    } else {
      LOG(WARNING) << "Services environment overridden with --" << switch_name
                   << "=\"" << value << "\"";
    }
  }
}

std::string GetGate3URL(base::CommandLine* command_line) {
  CHECK(base::CommandLine::InitializedForCurrentProcess());
  CHECK(command_line);

  // Prefix-specific switch takes precedence.
  std::string env = command_line->GetSwitchValueASCII(kEnvGate3Switch);
  MaybeWarnSwitchValue(kEnvGate3Switch, env);

  // Fall back to global services environment switch.
  if (env.empty() || !IsValidSwitchValue(env)) {
    env = command_line->GetSwitchValueASCII(kBraveServicesEnvironmentSwitch);
    MaybeWarnSwitchValue(kBraveServicesEnvironmentSwitch, env);
  }

  if (env == kBraveServicesSwitchValueDev ||
      env == kBraveServicesSwitchValueStaging) {
    return kGate3DevURL;
  }

  return kGate3ProdURL;
}

}  // namespace brave_domains
