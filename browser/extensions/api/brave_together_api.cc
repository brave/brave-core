/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_together_api.h"

#include <string>
#include <memory>

#include "base/environment.h"
#include "brave/components/brave_together/browser/regions.h"
#include "brave/components/ntp_widget_utils/browser/ntp_widget_utils_region.h"
#include "chrome/browser/profiles/profile.h"

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction
BraveTogetherIsSupportedFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());

  if (profile->IsTor()) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  bool is_supported = ntp_widget_utils::IsRegionSupported(
      profile->GetPrefs(), brave_together::unsupported_regions, false);
  return RespondNow(OneArgument(base::Value(is_supported)));
}

}  // namespace api
}  // namespace extensions
