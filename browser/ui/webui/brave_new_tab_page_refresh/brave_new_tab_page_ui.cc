// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page_ui.h"

#include <utility>

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/ntp_background/brave_ntp_custom_background_service_factory.h"
#include "brave/browser/ntp_background/custom_background_file_manager.h"
#include "brave/browser/ntp_background/ntp_p3a_helper_impl.h"
#include "brave/browser/ntp_background/view_counter_service_factory.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/background_facade.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/custom_image_chooser.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/new_tab_page_handler.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/new_tab_page_initializer.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_rich_media_ad_event_handler.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"

namespace {

using brave_new_tab_page_refresh::BackgroundFacade;
using brave_new_tab_page_refresh::CustomImageChooser;
using brave_new_tab_page_refresh::NewTabPageHandler;
using brave_new_tab_page_refresh::NewTabPageInitializer;

}  // namespace

BraveNewTabPageUI::BraveNewTabPageUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui) {
  NewTabPageInitializer(*web_ui).Initialize();
}

BraveNewTabPageUI::~BraveNewTabPageUI() = default;

void BraveNewTabPageUI::BindInterface(
    mojo::PendingReceiver<brave_new_tab_page_refresh::mojom::NewTabPageHandler>
        receiver) {
  auto* web_contents = web_ui()->GetWebContents();
  auto* profile = Profile::FromWebUI(web_ui());
  auto* prefs = profile->GetPrefs();
  auto image_chooser =
      std::make_unique<CustomImageChooser>(*web_contents, *profile);
  auto background_facade = std::make_unique<BackgroundFacade>(
      std::make_unique<CustomBackgroundFileManager>(profile), *prefs,
      g_brave_browser_process->ntp_background_images_service(),
      ntp_background_images::ViewCounterServiceFactory::GetForProfile(profile));

  page_handler_ = std::make_unique<NewTabPageHandler>(
      std::move(receiver), std::move(image_chooser),
      std::move(background_facade), *prefs);
}

void BraveNewTabPageUI::BindInterface(
    mojo::PendingReceiver<
        ntp_background_images::mojom::SponsoredRichMediaAdEventHandler>
        receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  std::unique_ptr<ntp_background_images::NTPP3AHelperImpl> ntp_p3a_helper;
  if (g_brave_browser_process->p3a_service()) {
    ntp_p3a_helper = std::make_unique<ntp_background_images::NTPP3AHelperImpl>(
        g_browser_process->local_state(),
        g_brave_browser_process->p3a_service(),
        g_brave_browser_process->ntp_background_images_service(),
        profile->GetPrefs());
  }
  rich_media_ad_event_handler_ = std::make_unique<
      ntp_background_images::NTPSponsoredRichMediaAdEventHandler>(
      brave_ads::AdsServiceFactory::GetForProfile(profile),
      std::move(ntp_p3a_helper));
}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveNewTabPageUI)
