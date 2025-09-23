// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEB_VIEW_AUTOFILL_BRAVE_WEB_VIEW_AUTOFILL_CLIENT_H_
#define BRAVE_IOS_BROWSER_API_WEB_VIEW_AUTOFILL_BRAVE_WEB_VIEW_AUTOFILL_CLIENT_H_

#include <memory>
#include <string>

#include "components/autofill/ios/browser/autofill_client_ios.h"
#include "ios/web_view/internal/autofill/web_view_autofill_client_ios.h"

namespace autofill {

// An autofill client for BraveWebView's
//
// We create a Brave subclass of the standard WebView autofill client to allow
// us to create one using Chrome Profile's rather than WebViewBrowserState and
// to ensure we implement GetAppLocale correcty using the main Chrome
// application context.
class BraveWebViewAutofillClientIOS : public WebViewAutofillClientIOS {
 public:
  static std::unique_ptr<WebViewAutofillClientIOS> Create(
      web::WebState* web_state,
      id<CWVAutofillClientIOSBridge, AutofillDriverIOSBridge> bridge);

  using WebViewAutofillClientIOS::WebViewAutofillClientIOS;

  const std::string& GetAppLocale() const override;
};
}  // namespace autofill

#endif  // BRAVE_IOS_BROWSER_API_WEB_VIEW_AUTOFILL_BRAVE_WEB_VIEW_AUTOFILL_CLIENT_H_
