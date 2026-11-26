// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_RECAP_BRAVE_RECAP_UI_IOS_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_RECAP_BRAVE_RECAP_UI_IOS_H_

#include "ios/web/public/webui/web_ui_ios_controller.h"

class GURL;

namespace web {
class WebUIIOS;
}  // namespace web

// brave://recap: a shareable summary of the profile's lifetime Shields stats.
class BraveRecapUIIOS : public web::WebUIIOSController {
 public:
  BraveRecapUIIOS(web::WebUIIOS* web_ui, const GURL& url);
  ~BraveRecapUIIOS() override;

  BraveRecapUIIOS(const BraveRecapUIIOS&) = delete;
  BraveRecapUIIOS& operator=(const BraveRecapUIIOS&) = delete;
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_RECAP_BRAVE_RECAP_UI_IOS_H_
