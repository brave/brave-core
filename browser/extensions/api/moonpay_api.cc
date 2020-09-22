/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/moonpay_api.h"

#include <string>
#include <memory>

#include "base/environment.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/moonpay/browser/regions.h"
#include "brave/components/moonpay/common/moonpay_pref_names.h"
#include "brave/components/ntp_widget_utils/browser/ntp_widget_utils_region.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction
MoonpayIsBitcoinDotComSupportedFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());

  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  bool is_supported = ntp_widget_utils::IsRegionSupported(
      profile->GetPrefs(), moonpay::bitcoin_dot_com_supported_regions, true);
  return RespondNow(OneArgument(
      std::make_unique<base::Value>(is_supported)));
}

ExtensionFunction::ResponseAction
MoonpayOnBuyBitcoinDotComCryptoFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());

  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  profile->GetPrefs()->SetBoolean(kMoonpayHasBoughtBitcoinDotComCrypto, true);

  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
