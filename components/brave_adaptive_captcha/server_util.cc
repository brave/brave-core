/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_adaptive_captcha/server_util.h"

#include <vector>

#include "base/check.h"
#include "base/command_line.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "bat/ledger/buildflags.h"
#include "brave/components/brave_ads/common/switches.h"

namespace brave_adaptive_captcha {

namespace {

enum class EnvironmentType { kStaging = 0, kProduction };

EnvironmentType ParseEnvironment(const std::string& switch_value) {
  EnvironmentType environment = EnvironmentType::kProduction;

  if (switch_value.empty()) {
    return environment;
  }

  const std::vector<std::string> flags =
      base::SplitString(base::ToLowerASCII(switch_value), ",",
                        base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  for (const auto& flag : flags) {
    const std::vector<std::string> components = base::SplitString(
        flag, "=", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    if (components.size() != 2) {
      continue;
    }

    const std::string& name = components.at(0);
    if (name == "staging") {
      const std::string& value = components.at(1);
      if (value == "true" || value == "1") {
        environment = EnvironmentType::kStaging;
      }

      break;
    }
  }

  return environment;
}

std::string GetHost(const EnvironmentType type) {
  switch (type) {
    case EnvironmentType::kStaging: {
      return BUILDFLAG(REWARDS_GRANT_STAGING_ENDPOINT);
    }

    case EnvironmentType::kProduction: {
      return BUILDFLAG(REWARDS_GRANT_PROD_ENDPOINT);
    }
  }
}

}  // namespace

std::string GetServerUrl(const std::string& path) {
  DCHECK(!path.empty());

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  const std::string switch_value =
      command_line.GetSwitchValueASCII(brave_ads::switches::kRewards);

  const EnvironmentType environment_type = ParseEnvironment(switch_value);

  return GetHost(environment_type) + path;
}

}  // namespace brave_adaptive_captcha
