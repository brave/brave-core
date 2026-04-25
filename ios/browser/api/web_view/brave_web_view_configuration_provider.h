// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_CONFIGURATION_PROVIDER_H_
#define BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_CONFIGURATION_PROVIDER_H_

#include "base/memory/raw_ptr.h"
#include "base/supports_user_data.h"

@class BraveWebViewConfiguration;

namespace web {
class BrowserState;
}

// A provider class associated with a single web::BrowserState object. Manages
// the lifetime and performs setup of BraveWebViewConfiguration and instances.
class BraveWebViewConfigurationProvider : public base::SupportsUserData::Data {
 public:
  // Returns a provider for the given `browser_state`. Lazily attaches one if it
  // does not exist. `browser_state` can not be null.
  static BraveWebViewConfigurationProvider& FromBrowserState(
      web::BrowserState* browser_state);
  ~BraveWebViewConfigurationProvider() override;

  // Returns the configuration or creates one if one hasn't already been made
  BraveWebViewConfiguration* GetConfiguration();
  // Destroys the current configuration
  void ResetConfiguration();

 private:
  explicit BraveWebViewConfigurationProvider(web::BrowserState* browser_state);

  raw_ptr<web::BrowserState> browser_state_;
  BraveWebViewConfiguration* configuration_ = nullptr;
};

#endif  // BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_CONFIGURATION_PROVIDER_H_
