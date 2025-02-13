// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_ANDROID_NEW_TAB_TAKEOVER_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_ANDROID_NEW_TAB_TAKEOVER_UI_H_

#include <memory>
#include <string>

#include "brave/components/brave_new_tab_ui/new_tab_takeover/mojom/new_tab_takeover.mojom.h"
#include "brave/components/ntp_background_images/browser/mojom/ntp_background_images.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

class PrefService;

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

class NewTabTakeoverUI : public ui::MojoWebUIController,
                         public new_tab_takeover::mojom::NewTabTakeover {
 public:
  NewTabTakeoverUI(content::WebUI* const web_ui,
                   const std::string& name,
                   brave_ads::AdsService* ads_service,
                   PrefService* local_state,
                   p3a::P3AService* p3a_service,
                   ntp_background_images::NTPBackgroundImagesService*
                       ntp_background_images_service,
                   PrefService* profile_prefs);

  NewTabTakeoverUI(const NewTabTakeoverUI&) = delete;
  NewTabTakeoverUI& operator=(const NewTabTakeoverUI&) = delete;

  ~NewTabTakeoverUI() override;

  void BindInterface(
      mojo::PendingReceiver<new_tab_takeover::mojom::NewTabTakeover>
          pending_receiver);

 private:
  // new_tab_takeover::mojom::NewTabTakeover:
  void SetSponsoredRichMediaAdEventHandler(
      mojo::PendingReceiver<
          ntp_background_images::mojom::SponsoredRichMediaAdEventHandler>
          event_handler) override;
  void NavigateToUrl(const GURL& url) override;

  mojo::Receiver<new_tab_takeover::mojom::NewTabTakeover>
      new_tab_takeover_receiver_{this};

  std::unique_ptr<ntp_background_images::NTPSponsoredRichMediaAdEventHandler>
      rich_media_ad_event_handler_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_ANDROID_NEW_TAB_TAKEOVER_UI_H_
