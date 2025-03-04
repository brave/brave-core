// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_UI_H_

#include <memory>
#include <string>

#include "brave/components/brave_new_tab_ui/brave_new_tab_page.mojom.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "chrome/browser/ui/webui/searchbox/realbox_handler.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"
#include "ui/webui/resources/cr_components/searchbox/searchbox.mojom.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"  // nogncheck
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

namespace brave_ads {
class AdsService;
}  // namespace brave_ads

namespace ntp_background_images {
class NTPBackgroundImagesService;
class NTPSponsoredRichMediaAdEventHandler;
}  // namespace ntp_background_images

namespace p3a {
class P3AService;
}  // namespace p3a

class BraveNewTabPageHandler;

class BraveNewTabUI : public ui::MojoWebUIController,
                      public brave_new_tab_page::mojom::PageHandlerFactory {
 public:
  BraveNewTabUI(content::WebUI* web_ui,
                const std::string& name,
                brave_ads::AdsService* ads_service,
                PrefService* local_state,
                p3a::P3AService* p3a_service,
                ntp_background_images::NTPBackgroundImagesService*
                    ntp_background_images_service);
  ~BraveNewTabUI() override;
  BraveNewTabUI(const BraveNewTabUI&) = delete;
  BraveNewTabUI& operator=(const BraveNewTabUI&) = delete;

  // Instantiates the implementor of the mojo
  // interface passing the pending receiver that will be internally bound.
  void BindInterface(
      mojo::PendingReceiver<brave_news::mojom::BraveNewsController> receiver);

  void BindInterface(
      mojo::PendingReceiver<brave_new_tab_page::mojom::PageHandlerFactory>
          pending_receiver);

  void BindInterface(mojo::PendingReceiver<searchbox::mojom::PageHandler>
                         pending_page_handler);

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  void BindInterface(mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler>
                         pending_vpn_service_handler);
#endif

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

  std::unique_ptr<BraveNewTabPageHandler> page_handler_;
  std::unique_ptr<RealboxHandler> realbox_handler_;
  mojo::Receiver<brave_new_tab_page::mojom::PageHandlerFactory>
      page_factory_receiver_;
  std::unique_ptr<ntp_background_images::NTPSponsoredRichMediaAdEventHandler>
      rich_media_ad_event_handler_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_UI_H_
