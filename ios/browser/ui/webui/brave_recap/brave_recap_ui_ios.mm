// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_recap/brave_recap_ui_ios.h"

#include <cstdint>

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/browser/resources/brave_recap/grit/brave_recap_generated_map.h"
#include "brave/browser/resources/brave_recap/grit/brave_recap_static_resources.h"
#include "brave/browser/resources/brave_recap/grit/brave_recap_static_resources_map.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/ios/web/webui/brave_web_ui_ios_data_source.h"
#include "brave/ios/web/webui/brave_webui_utils.h"
#include "components/grit/brave_components_webui_strings.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/webui/resources/grit/webui_resources.h"
#include "url/gurl.h"

namespace {

void AddStats(BraveWebUIIOSDataSource* source,
              PrefService* prefs,
              PrefService* local_state) {
  // Matches the NTP stats widget, which shows ads + trackers as one number.
  const uint64_t blocked =
      prefs->GetUint64(brave_shields::prefs::kAdsBlocked) +
      prefs->GetUint64(brave_shields::prefs::kTrackersBlocked);
  const uint64_t bandwidth_saved =
      prefs->GetUint64(brave_perf_predictor::prefs::kBandwidthSavedBytes);

  // 64-bit values do not fit loadTimeData integers, so pass as strings.
  source->AddString("adsBlocked", base::NumberToString(blocked));
  source->AddString("bandwidthSavedBytes",
                    base::NumberToString(bandwidth_saved));

  // iOS has no first run sentinel predating the Chromium integration, but this
  // pref is seeded from the app's original install date.
  const base::Time first_run =
      local_state->GetTime(brave_ads::prefs::kFirstRunAt);
  source->AddString(
      "firstRunTime",
      base::NumberToString(
          first_run.is_null() ? 0 : first_run.InMillisecondsSinceUnixEpoch()));
}

}  // namespace

BraveRecapUIIOS::BraveRecapUIIOS(web::WebUIIOS* web_ui, const GURL& url)
    : web::WebUIIOSController(web_ui, url.GetHost()) {
  BraveWebUIIOSDataSource* source = brave::CreateAndAddWebUIDataSource(
      web_ui, url.host(), kBraveRecapGenerated,
      IDR_BRAVE_RECAP_STATIC_BRAVE_RECAP_HTML);
  source->AddResourcePaths(kBraveRecapStaticResources);
  // The share card draws this onto a canvas, so it must be same-origin.
  source->AddResourcePath("brave-icon.svg",
                          IDR_BRAVE_ICONS_BRAVE_ICON_RELEASE_COLOR_SVG);
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src 'self' chrome://resources blob: data:;");
  source->AddLocalizedStrings(webui::kBraveRecapStrings);

  ProfileIOS* profile = ProfileIOS::FromWebUIIOS(web_ui)->GetOriginalProfile();
  AddStats(source, profile->GetPrefs(),
           GetApplicationContext()->GetLocalState());
}

BraveRecapUIIOS::~BraveRecapUIIOS() = default;
