// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_AUTOFILL_WEB_VIEW_AUTOFILL_CLIENT_IOS_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_AUTOFILL_WEB_VIEW_AUTOFILL_CLIENT_IOS_H_

#include <memory>

#include "components/autofill/core/browser/data_manager/personal_data_manager.h"
#include "components/autofill/ios/browser/autofill_client_ios.h"
#include "components/autofill/ios/browser/autofill_driver_ios_bridge.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/web_state.h"
#import "ios/web_view/internal/autofill/cwv_autofill_client_ios_bridge.h"

namespace autofill {

class WebViewAutofillClientIOS : public AutofillClientIOS {
 public:
  static std::unique_ptr<WebViewAutofillClientIOS> Create(
      FromWebStateImpl from_web_state_impl,
      web::WebState* web_state,
      id<CWVAutofillClientIOSBridge, AutofillDriverIOSBridge> bridge);

  void set_bridge(id<CWVAutofillClientIOSBridge> bridge) {}
};

}  // namespace autofill

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_AUTOFILL_WEB_VIEW_AUTOFILL_CLIENT_IOS_H_
