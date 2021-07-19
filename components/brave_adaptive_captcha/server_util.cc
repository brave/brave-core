/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_adaptive_captcha/server_util.h"

#include <utility>

#include "base/check.h"

namespace {

const char kDevelopment[] = "https://grant.rewards.brave.software";
const char kStaging[] = "https://grant.rewards.bravesoftware.com";
const char kProduction[] = "https://grant.rewards.brave.com";

}  // namespace

namespace brave_adaptive_captcha {

std::string GetServerUrl(brave_adaptive_captcha::Environment environment,
                         const std::string& path) {
  DCHECK(!path.empty());

  std::string url;
  switch (environment) {
    case brave_adaptive_captcha::Environment::DEVELOPMENT:
      url = kDevelopment;
      break;
    case brave_adaptive_captcha::Environment::STAGING:
      url = kStaging;
      break;
    case brave_adaptive_captcha::Environment::PRODUCTION:
      url = kProduction;
      break;
  }

  return url + path;
}

}  // namespace brave_adaptive_captcha
