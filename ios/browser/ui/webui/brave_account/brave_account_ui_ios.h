/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UI_IOS_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UI_IOS_H_

#include "brave/components/brave_account/brave_account_ui_base.h"
#include "brave/ios/browser/brave_account/brave_account_service_factory_ios.h"
#include "brave/ios/web/webui/brave_web_ui_ios_data_source.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"

class GURL;

namespace web {
class WebUIIOS;
}

class BraveAccountUIIOS
    : public BraveAccountUIBase<BraveWebUIIOSDataSource,
                                brave_account::BraveAccountServiceFactoryIOS>,
      public web::WebUIIOSController {
 public:
  BraveAccountUIIOS(web::WebUIIOS* web_ui, const GURL& url);

  ~BraveAccountUIIOS() override;
};

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_ACCOUNT_BRAVE_ACCOUNT_UI_IOS_H_
