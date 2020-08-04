/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_together_api.h"

#include <string>
#include <memory>

#include "base/environment.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/brave_together/browser/regions.h"
#include "brave/components/crypto_exchange/browser/crypto_exchange_region_util.h"
#include "chrome/browser/profiles/profile.h"

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction
BraveTogetherIsSupportedFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());

  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  bool is_supported = crypto_exchange::IsRegionSupported(
      profile->GetPrefs(), brave_together::supported_regions, true);
  return RespondNow(OneArgument(
      std::make_unique<base::Value>(is_supported)));
}

}  // namespace api
}  // namespace extensions
