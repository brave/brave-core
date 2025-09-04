/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_UI_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_UI_H_

#include "brave/components/brave_account/brave_account_ui_base.h"
#include "brave/ios/web/webui/brave_web_ui_ios_data_source.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"

class GURL;

namespace web {
class WebUIIOS;
class WebUIDataSource;
}

class NewTabTakeoverUI : public web::WebUIIOSController {
 public:
  NewTabTakeoverUI(web::WebUIIOS* web_ui, const GURL& url);

  ~NewTabTakeoverUI() override;

 private:
  // void SetupWebUIDataSource(WebUIDataSource* source);
  // void OverrideContentSecurityPolicy(WebUIDataSource* source);

 private:
  NewTabTakeoverHandler handler_;

  base::WeakPtrFactory<NewTabTakeoverUI> weak_ptr_factory_{this};
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_NEW_TAB_TAKEOVER_UI_H_
