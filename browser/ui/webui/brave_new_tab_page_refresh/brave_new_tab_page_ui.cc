// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page_ui.h"

#include <utility>

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/browser/ntp_background/brave_ntp_custom_background_service_factory.h"
#include "brave/browser/ntp_background/custom_background_file_manager.h"
#include "brave/browser/ntp_background/ntp_p3a_helper_impl.h"
#include "brave/browser/ntp_background/view_counter_service_factory.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/background_facade.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/custom_image_chooser.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/new_tab_page_handler.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/new_tab_page_initializer.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/top_sites_facade.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/vpn_facade.h"
#include "brave/browser/ui/webui/brave_rewards/rewards_page_handler.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_rich_media_ad_event_handler.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ntp_tiles/chrome_most_visited_sites_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/webui/searchbox/realbox_handler.h"
#include "components/tab_collections/public/tab_interface.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#endif

namespace {

using brave_new_tab_page_refresh::BackgroundFacade;
using brave_new_tab_page_refresh::CustomImageChooser;
using brave_new_tab_page_refresh::NewTabPageHandler;
using brave_new_tab_page_refresh::NewTabPageInitializer;
using brave_new_tab_page_refresh::TopSitesFacade;
using brave_new_tab_page_refresh::VPNFacade;

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
  auto* tab = tabs::TabInterface::GetFromContents(web_contents);
  auto image_chooser =
      std::make_unique<CustomImageChooser>(*web_contents, *profile);
  auto background_facade = std::make_unique<BackgroundFacade>(
      std::make_unique<CustomBackgroundFileManager>(profile), *prefs,
      g_brave_browser_process->ntp_background_images_service(),
      ntp_background_images::ViewCounterServiceFactory::GetForProfile(profile));
  auto top_sites_facade = std::make_unique<TopSitesFacade>(
      ChromeMostVisitedSitesFactory::NewForProfile(profile), *prefs);

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  auto vpn_facade = std::make_unique<VPNFacade>(
      *tab, brave_vpn::BraveVpnServiceFactory::GetForProfile(profile));
#else
  auto vpn_facade = std::make_unique<VPNFacade>();
#endif

  page_handler_ = std::make_unique<NewTabPageHandler>(
      std::move(receiver), std::move(image_chooser),
      std::move(background_facade), std::move(top_sites_facade),
      std::move(vpn_facade), *tab, *prefs,
      *TemplateURLServiceFactory::GetForProfile(profile),
      *g_brave_browser_process->process_misc_metrics()->new_tab_metrics());
}

void BraveNewTabPageUI::BindInterface(
    mojo::PendingReceiver<
        ntp_background_images::mojom::SponsoredRichMediaAdEventHandler>
        receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  ntp_background_images::NTPP3AHelper* ntp_p3a_helper = nullptr;
  if (ntp_background_images::ViewCounterService* view_counter_service =
          ntp_background_images::ViewCounterServiceFactory::GetForProfile(
              profile)) {
    ntp_p3a_helper = view_counter_service->GetP3AHelper();
  }
  rich_media_ad_event_handler_ = std::make_unique<
      ntp_background_images::NTPSponsoredRichMediaAdEventHandler>(
      brave_ads::AdsServiceFactory::GetForProfile(profile), ntp_p3a_helper);
  rich_media_ad_event_handler_->Bind(std::move(receiver));
}

void BraveNewTabPageUI::BindInterface(
    mojo::PendingReceiver<searchbox::mojom::PageHandler> receiver) {
  realbox_handler_ = std::make_unique<RealboxHandler>(
      std::move(receiver), Profile::FromWebUI(web_ui()),
      web_ui()->GetWebContents(), /*metrics_reporter=*/nullptr,
      /*omnibox_controller=*/nullptr);
}

void BraveNewTabPageUI::BindInterface(
    mojo::PendingReceiver<brave_rewards::mojom::RewardsPageHandler> receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  rewards_page_handler_ = std::make_unique<brave_rewards::RewardsPageHandler>(
      std::move(receiver), nullptr,
      brave_rewards::RewardsServiceFactory::GetForProfile(profile),
      brave_ads::AdsServiceFactory::GetForProfile(profile), nullptr,
      profile->GetPrefs());
}

#if BUILDFLAG(ENABLE_BRAVE_VPN)
void BraveNewTabPageUI::BindInterface(
    mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler> receiver) {
  auto* vpn_service = brave_vpn::BraveVpnServiceFactory::GetForProfile(
      Profile::FromWebUI(web_ui()));
  if (vpn_service) {
    vpn_service->BindInterface(std::move(receiver));
  }
}
#endif

WEB_UI_CONTROLLER_TYPE_IMPL(BraveNewTabPageUI)
