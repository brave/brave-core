// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BRAVE_DOMAINS_CONSTANTS_H_
#define BRAVE_BRAVE_DOMAINS_CONSTANTS_H_

#include <string>

#include "base/command_line.h"

namespace brave_domains {

// Common CLI switch names and values for environment overrides.
inline constexpr char kBraveServicesEnvironmentSwitch[] = "brave-services-env";
inline constexpr char kEnvGate3Switch[] = "env-gate3";
inline constexpr char kBraveServicesSwitchValueDev[] = "dev";
inline constexpr char kBraveServicesSwitchValueStaging[] = "staging";
inline constexpr char kBraveServicesSwitchValueProduction[] = "prod";

// Returns true if the value is a recognized environment switch value.
bool IsValidSwitchValue(const std::string& value);

// Logs an error if the switch value is set but not recognized.
void MaybeWarnSwitchValue(const std::string& switch_name,
                          const std::string& value);

// Returns the gate3 URL, checking --env-gate3 and --brave-services-env
// CLI switches on the provided command line:
//   DEV/STAGING: https://gate3.wallet.brave.software
//   PRODUCTION:  https://gate3.wallet.brave.com
std::string GetGate3URL(
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess());

}  // namespace brave_domains

#endif  // BRAVE_BRAVE_DOMAINS_CONSTANTS_H_
