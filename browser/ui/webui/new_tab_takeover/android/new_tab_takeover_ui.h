// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_ANDROID_NEW_TAB_TAKEOVER_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_ANDROID_NEW_TAB_TAKEOVER_UI_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/new_tab_takeover/mojom/new_tab_takeover.mojom.h"
#include "brave/components/ntp_background_images/browser/mojom/ntp_background_images.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace ntp_background_images {
class NTPSponsoredRichMediaAdEventHandler;
class ViewCounterService;
}  // namespace ntp_background_images

// On desktop, we use a Web UI to display new tab pages. On Android, however,
// there is no Web UI implementation. Instead, Android overlays a native view
// over a web contents view. The native view displays the background image,
// Brave Stats, and Brave News. When the user navigates to a URL, the native
// view is hidden, revealing the web contents view and its HTML content. To
// display rich media HTML alongside Brave Stats and Brave News, we use a
// `ThinWebView` to render the HTML behind these overlays.
class NewTabTakeoverUI : public ui::MojoWebUIController,
                         public new_tab_takeover::mojom::NewTabTakeover {
 public:
  NewTabTakeoverUI(
      content::WebUI* const web_ui,
      ntp_background_images::ViewCounterService* view_counter_service,
      std::unique_ptr<
          ntp_background_images::NTPSponsoredRichMediaAdEventHandler>
          rich_media_ad_event_handler);

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
  void GetCurrentWallpaper(GetCurrentWallpaperCallback callback) override;
  void NavigateToUrl(const GURL& url) override;

  mojo::Receiver<new_tab_takeover::mojom::NewTabTakeover>
      new_tab_takeover_receiver_{this};

  raw_ptr<ntp_background_images::ViewCounterService>
      view_counter_service_;  // Not owned.

  std::unique_ptr<ntp_background_images::NTPSponsoredRichMediaAdEventHandler>
      rich_media_ad_event_handler_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_ANDROID_NEW_TAB_TAKEOVER_UI_H_
