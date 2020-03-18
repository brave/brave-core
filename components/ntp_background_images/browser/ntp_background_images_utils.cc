// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/ntp_background_images_utils.h"

#include <string>

#include "base/files/file_path.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"

namespace ntp_background_images {

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kNewTabPageCachedReferralPromoCode,
                               std::string());
  registry->RegisterDictionaryPref(
      prefs::kNewTabPageCachedSuperReferrerComponentInfo);
}

std::unique_ptr<NTPBackgroundImagesData> GetDemoWallpaper(bool super_referrer) {
  auto demo = std::make_unique<NTPBackgroundImagesData>();
  demo->url_prefix = "chrome://newtab/ntp-dummy-brandedwallpaper/";
  demo->backgrounds = {
      { base::FilePath(FILE_PATH_LITERAL("wallpaper1.jpg")), { 3988, 2049 } },
      { base::FilePath(FILE_PATH_LITERAL("wallpaper2.jpg")), { 5233, 3464 } },
      { base::FilePath(FILE_PATH_LITERAL("wallpaper3.jpg")), {  0, 0 } },
  };
  demo->logo_alt_text = "Technikke: For music lovers.";
  demo->logo_company_name = "Technikke";
  demo->logo_destination_url = "https://brave.com";

  if (super_referrer) {
    demo->top_sites = {
      { "Brave", "https://brave.com", "brave.png",
        base::FilePath(FILE_PATH_LITERAL("brave.png")) },
      { "BAT", "https://basicattentiontoken.org/", "bat.png",
        base::FilePath(FILE_PATH_LITERAL("bat.png")) },
    };
  }

  return demo;
}

}  // namespace ntp_background_images
