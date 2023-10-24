// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/brave_domains/service_domains.h"

#include <string>

#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "brave/brave_domains/buildflags.h"

namespace brave_domains {

namespace {

const char kBraveServicesEnvironmentSwitch[] = "brave-services-env";
const char kBraveServicesSwitchValueStaging[] = "staging";
const char kBraveServicesSwitchValueDev[] = "dev";
const char kBraveServicesSwitchValueProduction[] = "prod";

std::string GetServicesDomainForSwitchValue(std::string env_from_switch) {
  if (env_from_switch == kBraveServicesSwitchValueStaging) {
    return BUILDFLAG(BRAVE_SERVICES_STAGING_DOMAIN);
  }
  if (env_from_switch == kBraveServicesSwitchValueDev) {
    return BUILDFLAG(BRAVE_SERVICES_DEV_DOMAIN);
  }
  // Default to production
  return BUILDFLAG(BRAVE_SERVICES_PRODUCTION_DOMAIN);
}

bool IsValidSwitchValue(std::string value) {
  static const auto kAllowedSwitchValues =
      base::MakeFixedFlatSet<std::string_view>(
          {kBraveServicesSwitchValueDev, kBraveServicesSwitchValueStaging,
           kBraveServicesSwitchValueProduction});
  return base::Contains(kAllowedSwitchValues, value);
}

void MaybeWarnSwitchValue(std::string key, std::string value) {
  if (!value.empty()) {
    if (!IsValidSwitchValue(value)) {
      LOG(ERROR) << "The switch value for --" << key << " is \"" << value
                 << "\" which is not a valid value, please provide"
                 << " either \"" << kBraveServicesSwitchValueDev << "\", \""
                 << kBraveServicesSwitchValueStaging << "\", or \""
                 << kBraveServicesSwitchValueProduction << "\" (default).";
    } else {
      // It's useful to have this in the logs. This should be a temporary
      // dev or debug tool and not a permanent situation for a user.
      LOG(WARNING) << "Services domain(s) was overriden with the parameter: --"
                   << key << " and value \"" << value << "\"";
    }
  }
}

}  // namespace

std::string GetServicesDomain(
    std::string prefix /* = "" */,
    base::CommandLine*
        command_line /* = base::CommandLine::ForCurrentProcess()*/) {
  std::string env_key;
  std::string env_from_switch;

  // Dynamic key allows overriding environment for just a subdomain prefix
  if (!prefix.empty()) {
    env_key = base::StrCat({"env-", prefix});
    env_from_switch = command_line->GetSwitchValueASCII(env_key);
    MaybeWarnSwitchValue(env_key, env_from_switch);
  }

  // When not overriden or invalid, use global default or override
  if (env_key.empty() || env_from_switch.empty() ||
      !IsValidSwitchValue(env_from_switch)) {
    env_from_switch =
        command_line->GetSwitchValueASCII(kBraveServicesEnvironmentSwitch);
    MaybeWarnSwitchValue(kBraveServicesEnvironmentSwitch, env_from_switch);
    // Handle global default is not overriden or is invalid
    if (!env_from_switch.empty() && !IsValidSwitchValue(env_from_switch)) {
      env_from_switch.clear();
    }
  }

  // Build hostname

  if (prefix.empty()) {
    return GetServicesDomainForSwitchValue(env_from_switch);
  }

  return base::StrCat(
      {prefix, ".", GetServicesDomainForSwitchValue(env_from_switch)});
}

}  // namespace brave_domains
