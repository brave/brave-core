// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web_view/internal/autofill/web_view_autofill_client_ios.h"

namespace autofill {

// static
std::unique_ptr<WebViewAutofillClientIOS> WebViewAutofillClientIOS::Create(
    web::WebState* web_state,
    web::BrowserState* browser_state) {
  return nullptr;
}

}  // namespace autofill
