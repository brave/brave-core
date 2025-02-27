/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_UI_WK_WEB_VIEW_CONFIGURATION_PROVIDER_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_UI_WK_WEB_VIEW_CONFIGURATION_PROVIDER_H_

// Support for subclassing WKWebViewConfigurationProvider:
// - Add a friend class as the subclass needs access to private fields
// (`configuration_` in this case)
// - Make `ResetWithWebViewConfiguration` virtual so that we can override it
#define browser_state_ \
  browser_state_;      \
  friend class BraveWKWebViewConfigurationProvider
#define ResetWithWebViewConfiguration \
  Unused() {}                         \
  virtual void ResetWithWebViewConfiguration
#include "src/ios/web/web_state/ui/wk_web_view_configuration_provider.h"  // IWYU pragma: export
#undef ResetWithWebViewConfiguration
#undef browser_state_

namespace web {
class BraveWKWebViewConfigurationProvider
    : public WKWebViewConfigurationProvider {
  using WKWebViewConfigurationProvider::WKWebViewConfigurationProvider;

  void ResetWithWebViewConfiguration(
      WKWebViewConfiguration* configuration) override;
};
}  // namespace web

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_UI_WK_WEB_VIEW_CONFIGURATION_PROVIDER_H_
