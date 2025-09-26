// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEB_VIEW_PASSWORDS_BRAVE_WEB_VIEW_PASSWORD_MANAGER_CLIENT_H_
#define BRAVE_IOS_BROWSER_API_WEB_VIEW_PASSWORDS_BRAVE_WEB_VIEW_PASSWORD_MANAGER_CLIENT_H_

#include "ios/web_view/internal/passwords/web_view_password_manager_client.h"

// An password manager client for BraveWebView's
//
// We create a Brave subclass of the standard WebView pasword manager client
// to allow to ensure we implement GetLocalStatePrefs correcty using the main
// Chrome application context.
class BraveWebViewPasswordManagerClient
    : public ios_web_view::WebViewPasswordManagerClient {
 public:
  using WebViewPasswordManagerClient::WebViewPasswordManagerClient;

  // password_manager::PasswordManagerClient implementation.
  PrefService* GetLocalStatePrefs() const override;
};

#endif  // BRAVE_IOS_BROWSER_API_WEB_VIEW_PASSWORDS_BRAVE_WEB_VIEW_PASSWORD_MANAGER_CLIENT_H_
