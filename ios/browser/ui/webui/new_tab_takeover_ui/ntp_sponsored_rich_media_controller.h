/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_UI_NTP_SPONSORED_RICH_MEDIA_CONTROLLER_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_UI_NTP_SPONSORED_RICH_MEDIA_CONTROLLER_H_

#include "ios/web/public/webui/web_ui_ios_controller.h"

namespace ntp_background_images {
class NTPBackgroundImagesService;
}  // namespace ntp_background_images

class NTPSponsoredRichMediaController : public web::WebUIIOSController {
 public:
  NTPSponsoredRichMediaController(
      web::WebUIIOS* web_ui,
      const GURL& url,
      ntp_background_images::NTPBackgroundImagesService*
          background_images_service);

  ~NTPSponsoredRichMediaController() override;
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_UI_NTP_SPONSORED_RICH_MEDIA_CONTROLLER_H_
