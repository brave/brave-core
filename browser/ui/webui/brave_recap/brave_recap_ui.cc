// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_recap/brave_recap_ui.h"

#include <cstdint>

#include "base/feature_list.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/browser/brave_stats/first_run_util.h"
#include "brave/browser/resources/brave_recap/grit/brave_recap_generated_map.h"
#include "brave/browser/resources/brave_recap/grit/brave_recap_static_resources.h"
#include "brave/browser/resources/brave_recap/grit/brave_recap_static_resources_map.h"
#include "brave/browser/ui/brave_ui_features.h"
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_webui_strings.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/webui/webui_util.h"

namespace {

void AddStats(content::WebUIDataSource* source, PrefService* prefs) {
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

  const base::Time first_run =
      brave_stats::GetFirstRunTime(g_browser_process->local_state());
  source->AddString(
      "firstRunTime",
      base::NumberToString(
          first_run.is_null() ? 0 : first_run.InMillisecondsSinceUnixEpoch()));
}

}  // namespace

BraveRecapUI::BraveRecapUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(profile, kBraveRecapHost);
  webui::SetupWebUIDataSource(source, kBraveRecapGenerated,
                              IDR_BRAVE_RECAP_STATIC_BRAVE_RECAP_HTML);
  source->AddResourcePaths(kBraveRecapStaticResources);
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src 'self' chrome://resources blob: data:;");
  source->AddLocalizedStrings(webui::kBraveRecapStrings);
  AddStats(source, profile->GetOriginalProfile()->GetPrefs());
}

BraveRecapUI::~BraveRecapUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(BraveRecapUI)

bool BraveRecapUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return base::FeatureList::IsEnabled(features::kBraveRecap);
}
