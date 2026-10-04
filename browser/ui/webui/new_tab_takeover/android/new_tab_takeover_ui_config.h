// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_ANDROID_NEW_TAB_TAKEOVER_UI_CONFIG_H_
#define BRAVE_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_ANDROID_NEW_TAB_TAKEOVER_UI_CONFIG_H_

#include <memory>

#include "content/public/browser/webui_config.h"

class GURL;

namespace content {
class WebUI;
class WebUIController;
}  // namespace content

class NewTabTakeoverUIConfig : public content::WebUIConfig {
 public:
  NewTabTakeoverUIConfig();

  // content::WebUIConfig:
  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_ANDROID_NEW_TAB_TAKEOVER_UI_CONFIG_H_
