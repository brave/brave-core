/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_adaptive_captcha/server_util.h"

#include <utility>

#include "base/check.h"
#include "base/command_line.h"
#include "base/notreached.h"
#include "brave/components/brave_ads/common/switches.h"

namespace brave_adaptive_captcha {

std::string GetServerUrl(const std::string& path) {
  DCHECK(!path.empty());

  std::string url = REWARDS_GRANT_PROD_ENDPOINT;

  auto* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(brave_ads::switches::kDevelopment)) {
    url = REWARDS_GRANT_DEV_ENDPOINT;
  } else if (command_line->HasSwitch(brave_ads::switches::kStaging)) {
    url = REWARDS_GRANT_STAGING_ENDPOINT;
  } else if (command_line->HasSwitch(brave_ads::switches::kProduction)) {
    url = REWARDS_GRANT_PROD_ENDPOINT;
  }

  return url + path;
}

}  // namespace brave_adaptive_captcha
