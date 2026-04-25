// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_UI_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_new_tab_ui/brave_new_tab_page.mojom.h"
#include "brave/components/brave_news/common/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "components/omnibox/browser/searchbox.mojom.h"
#include "components/regional_capabilities/regional_capabilities_service.h"
#include "content/public/browser/web_ui_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
#include "brave/components/brave_news/common/brave_news.mojom.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"  // nogncheck
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

namespace brave_ads {
class AdsService;
}  // namespace brave_ads

namespace ntp_background_images {
class NTPSponsoredRichMediaAdEventHandler;
class ViewCounterService;
}  // namespace ntp_background_images

namespace contextual_search {
class ContextualSearchSessionHandle;
}  // namespace contextual_search

class RealboxHandler;

class BraveNewTabPageHandler;

class BraveNewTabUI : public ui::MojoWebUIController,
                      public brave_new_tab_page::mojom::PageHandlerFactory {
 public:
  BraveNewTabUI(content::WebUI* web_ui,
                std::string_view name,
                brave_ads::AdsService* ads_service,
                ntp_background_images::ViewCounterService* view_counter_service,
                regional_capabilities::RegionalCapabilitiesService*
                    regional_capabilities);
  ~BraveNewTabUI() override;
  BraveNewTabUI(const BraveNewTabUI&) = delete;
  BraveNewTabUI& operator=(const BraveNewTabUI&) = delete;

  // Instantiates the implementor of the mojo
  // interface passing the pending receiver that will be internally bound.
#if BUILDFLAG(ENABLE_BRAVE_NEWS)
  void BindInterface(
      mojo::PendingReceiver<brave_news::mojom::BraveNewsController> receiver);
#endif

  void BindInterface(
      mojo::PendingReceiver<brave_new_tab_page::mojom::PageHandlerFactory>
          pending_receiver);

  void BindInterface(mojo::PendingReceiver<searchbox::mojom::PageHandler>
                         pending_page_handler);

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  void BindInterface(mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler>
                         pending_vpn_service_handler);
#endif

  // Returns a reference to the owned contextual search session handle for
  // `realbox_handler_`.
  contextual_search::ContextualSearchSessionHandle*
  GetContextualSessionHandle();

 private:
  // new_tab_page::mojom::PageHandlerFactory:
  void CreatePageHandler(
      mojo::PendingRemote<brave_new_tab_page::mojom::Page> pending_page,
      mojo::PendingReceiver<brave_new_tab_page::mojom::PageHandler>
          pending_page_handler,
      mojo::PendingReceiver<brave_new_tab_page::mojom::NewTabMetrics>
          pending_new_tab_metrics,
      mojo::PendingReceiver<
          ntp_background_images::mojom::SponsoredRichMediaAdEventHandler>
          pending_rich_media_ad_event_handler) override;

  // Must outlive `realbox_handler_`.
  std::unique_ptr<contextual_search::ContextualSearchSessionHandle>
      session_handle_;

  std::unique_ptr<BraveNewTabPageHandler> page_handler_;
  std::unique_ptr<RealboxHandler> realbox_handler_;
  mojo::Receiver<brave_new_tab_page::mojom::PageHandlerFactory>
      page_factory_receiver_;
  std::unique_ptr<ntp_background_images::NTPSponsoredRichMediaAdEventHandler>
      rich_media_ad_event_handler_;
  raw_ptr<regional_capabilities::RegionalCapabilitiesService>
      regional_capabilities_ = nullptr;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_UI_H_
