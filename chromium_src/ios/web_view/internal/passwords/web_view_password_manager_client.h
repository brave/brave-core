// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_PASSWORDS_WEB_VIEW_PASSWORD_MANAGER_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_PASSWORDS_WEB_VIEW_PASSWORD_MANAGER_CLIENT_H_

#include <memory>

#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/ios/password_manager_client_bridge.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/web_state.h"

namespace ios_web_view {

class WebViewPasswordManagerClient
    : public password_manager::PasswordManagerClient {
 public:
  static std::unique_ptr<WebViewPasswordManagerClient> Create(
      web::WebState* web_state,
      web::BrowserState* browser_state);

  void set_bridge(id<PasswordManagerClientBridge> bridge) {}
};

}  // namespace ios_web_view

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_PASSWORDS_WEB_VIEW_PASSWORD_MANAGER_CLIENT_H_
