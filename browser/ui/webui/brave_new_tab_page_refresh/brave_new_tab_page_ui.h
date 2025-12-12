// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_BRAVE_NEW_TAB_PAGE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_BRAVE_NEW_TAB_PAGE_UI_H_

#include <memory>

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page.mojom.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_rewards/core/mojom/rewards_page.mojom.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/ntp_background_images/browser/mojom/ntp_background_images.mojom.h"
#include "components/omnibox/browser/searchbox.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#endif

namespace ntp_background_images {
class NTPSponsoredRichMediaAdEventHandler;
}

namespace brave_rewards {
class RewardsPageHandler;
}

class RealboxHandler;

// The Web UI controller for the Brave new tab page.
class BraveNewTabPageUI : public ui::MojoWebUIController {
 public:
  explicit BraveNewTabPageUI(content::WebUI* web_ui);
  ~BraveNewTabPageUI() override;

  void BindInterface(
      mojo::PendingReceiver<
          brave_new_tab_page_refresh::mojom::NewTabPageHandler> receiver);

  void BindInterface(
      mojo::PendingReceiver<
          ntp_background_images::mojom::SponsoredRichMediaAdEventHandler>
          receiver);

  void BindInterface(
      mojo::PendingReceiver<searchbox::mojom::PageHandler> receiver);

  void BindInterface(
      mojo::PendingReceiver<brave_rewards::mojom::RewardsPageHandler> receiver);

  void BindInterface(
      mojo::PendingReceiver<brave_news::mojom::BraveNewsController> receiver);

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  void BindInterface(
      mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler> receiver);
#endif

 private:
  std::unique_ptr<brave_new_tab_page_refresh::mojom::NewTabPageHandler>
      page_handler_;
  std::unique_ptr<ntp_background_images::NTPSponsoredRichMediaAdEventHandler>
      rich_media_ad_event_handler_;
  std::unique_ptr<RealboxHandler> realbox_handler_;
  std::unique_ptr<brave_rewards::RewardsPageHandler> rewards_page_handler_;
  bool was_restored_ = false;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_BRAVE_NEW_TAB_PAGE_UI_H_
