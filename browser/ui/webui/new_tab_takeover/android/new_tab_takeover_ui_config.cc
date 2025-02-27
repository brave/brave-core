// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_takeover/android/new_tab_takeover_ui_config.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/ntp_background/ntp_p3a_helper_impl.h"
#include "brave/browser/ntp_background/view_counter_service_factory.h"
#include "brave/browser/ui/webui/new_tab_takeover/android/new_tab_takeover_ui.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ntp_background_images/browser/ntp_p3a_helper.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_rich_media_ad_event_handler.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"

NewTabTakeoverUIConfig::NewTabTakeoverUIConfig()
    : WebUIConfig(content::kChromeUIScheme, kNewTabTakeoverHost) {}

std::unique_ptr<content::WebUIController>
NewTabTakeoverUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                              const GURL& url) {
  Profile* profile = Profile::FromWebUI(web_ui);

  auto ntp_p3a_helper =
      std::make_unique<ntp_background_images::NTPP3AHelperImpl>(
          g_browser_process->local_state(),
          g_brave_browser_process->p3a_service(),
          g_brave_browser_process->ntp_background_images_service(),
          profile->GetPrefs());

  auto rich_media_ad_event_handler = std::make_unique<
      ntp_background_images::NTPSponsoredRichMediaAdEventHandler>(
      brave_ads::AdsServiceFactory::GetForProfile(profile),
      std::move(ntp_p3a_helper));

  return std::make_unique<NewTabTakeoverUI>(
      web_ui,
      ntp_background_images::ViewCounterServiceFactory::GetForProfile(profile),
      std::move(rich_media_ad_event_handler));
}
