// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BRAVE_DOMAINS_SERVICE_DOMAINS_H_
#define BRAVE_BRAVE_DOMAINS_SERVICE_DOMAINS_H_

#include <string>

#include "base/command_line.h"

namespace brave_domains {

constexpr char kBraveServicesSwitchValueDev[] = "dev";
constexpr char kBraveServicesSwitchValueStaging[] = "staging";
constexpr char kBraveServicesSwitchValueProduction[] = "prod";

// Gets production services domain, or returns staging or dev
// domain if relevant cli parameter is present.
// When prefix is provided,
// CLI param syntax is env-[prefix]={dev,staging,prod}.
// Prefix should not end in the "." separator.
//
// When prefix is empty, returns base production services domain.
//
// All domains can be overridden globally via
// CLI param syntax is brave-services-env={dev,staging,prod}.
//
// Precedence is:
// 1. Prefix specific CLI overrides
// 2. Global CLI overrides overrides
// 3. Default env override parameter
// 4. Default env (production)
std::string GetServicesDomain(
    std::string prefix,
    std::string env_value_default_override = "",
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess());

}  // namespace brave_domains

#endif  // BRAVE_BRAVE_DOMAINS_SERVICE_DOMAINS_H_
