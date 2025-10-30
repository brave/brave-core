// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_takeover/android/new_tab_takeover_ui_config.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/ntp_background/view_counter_service_factory.h"
#include "brave/browser/ui/webui/new_tab_takeover/android/new_tab_takeover_ui.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_rich_media_ad_event_handler.h"
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

  auto rich_media_ad_event_handler = std::make_unique<
      ntp_background_images::NTPSponsoredRichMediaAdEventHandler>(
      brave_ads::AdsServiceFactory::GetForProfile(profile));

  ntp_background_images::NTPBackgroundImagesService*
      ntp_background_images_service =
          g_brave_browser_process->ntp_background_images_service();
  CHECK(ntp_background_images_service);

  return std::make_unique<NewTabTakeoverUI>(
      web_ui, *ntp_background_images_service,
      std::move(rich_media_ad_event_handler));
}
