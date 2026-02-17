// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_WEB_STATE_UI_BRAVE_WK_WEB_VIEW_CONFIGURATION_PROVIDER_H_
#define BRAVE_IOS_WEB_WEB_STATE_UI_BRAVE_WK_WEB_VIEW_CONFIGURATION_PROVIDER_H_

#include "ios/web/web_state/ui/wk_web_view_configuration_provider.h"

namespace brave {
web::WKWebViewConfigurationProvider* CreateBraveWKWebViewConfigurationProvider(
    web::BrowserState* browser_state);
}

namespace web {
class BraveWKWebViewConfigurationProvider
    : public WKWebViewConfigurationProvider {
 public:
  void ResetWithWebViewConfiguration(
      WKWebViewConfiguration* configuration) override;

 private:
  friend web::WKWebViewConfigurationProvider*
  brave::CreateBraveWKWebViewConfigurationProvider(web::BrowserState*);
  explicit BraveWKWebViewConfigurationProvider(
      web::BrowserState* browser_state);
};
}  // namespace web

#endif  // BRAVE_IOS_WEB_WEB_STATE_UI_BRAVE_WK_WEB_VIEW_CONFIGURATION_PROVIDER_H_
