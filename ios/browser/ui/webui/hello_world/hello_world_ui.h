// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_HELLO_WORLD_HELLO_WORLD_UI_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_HELLO_WORLD_HELLO_WORLD_UI_H_

#include "ios/web/public/webui/web_ui_ios_controller.h"

class GURL;

namespace web {
class WebUIIOS;
}

class HelloWorldUI : public web::WebUIIOSController {
 public:
  explicit HelloWorldUI(web::WebUIIOS* web_ui, const GURL& url);
  ~HelloWorldUI() override;
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_HELLO_WORLD_HELLO_WORLD_UI_H_
