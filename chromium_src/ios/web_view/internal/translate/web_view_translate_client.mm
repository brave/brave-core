// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web_view/internal/translate/web_view_translate_client.h"

namespace ios_web_view {

// static
std::unique_ptr<WebViewTranslateClient> WebViewTranslateClient::Create(
    web::BrowserState* browser_state,
    web::WebState* web_state) {
  return nullptr;
}

}  // namespace ios_web_view
