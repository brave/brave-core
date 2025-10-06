/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_UI_NEW_TAB_TAKEOVER_UI_IOS_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_UI_NEW_TAB_TAKEOVER_UI_IOS_H_

// #include "brave/ios/web/webui/brave_web_ui_ios_data_source.h"
#include "brave/components/new_tab_takeover/mojom/new_tab_takeover.mojom.h"
#include "brave/ios/browser/ui/webui/new_tab_takeover_ui/new_tab_takeover_ui_handler.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

class GURL;

namespace ntp_background_images {
class NTPBackgroundImagesService;
}  // namespace ntp_background_images

namespace web {
class WebUIIOS;
class WebUIDataSource;
}  // namespace web

class NewTabTakeoverUIIOS : public web::WebUIIOSController {
 public:
  NewTabTakeoverUIIOS(web::WebUIIOS* web_ui,
                      const GURL& url,
                      ntp_background_images::NTPBackgroundImagesService*
                          ntp_background_images_service);

  ~NewTabTakeoverUIIOS() override;

 private:
  void BindInterface(
      mojo::PendingReceiver<new_tab_takeover::mojom::NewTabTakeover>
          pending_receiver);

  // void SetupWebUIDataSource(WebUIDataSource* source);
  // void OverrideContentSecurityPolicy(WebUIDataSource* source);

 private:
  NewTabTakeoverUIHandler handler_;

  base::WeakPtrFactory<NewTabTakeoverUIIOS> weak_ptr_factory_{this};
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_UI_NEW_TAB_TAKEOVER_UI_IOS_H_
