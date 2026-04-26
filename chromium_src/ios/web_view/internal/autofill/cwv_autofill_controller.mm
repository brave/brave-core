// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"

#include <memory>

#include "base/functional/callback_forward.h"
#include "components/autofill/ios/browser/autofill_client_ios.h"
#include "ios/web/public/web_state.h"
#include "ios/web_view/internal/autofill/cwv_autofill_client_ios_bridge.h"
#include "ios/web_view/internal/autofill/cwv_autofill_controller_internal.h"
#include "ios/web_view/internal/autofill/web_view_autofill_client_ios.h"

#include <ios/web_view/internal/autofill/cwv_autofill_controller.mm>

@implementation CWVAutofillController (Internal)
- (instancetype)
         initWithWebState:(web::WebState*)webState
     createAutofillClient:(CreateAutofillClientCallback)createAutofillClient
            autofillAgent:(AutofillAgent*)autofillAgent
          passwordManager:(std::unique_ptr<password_manager::PasswordManager>)
                              passwordManager
    passwordManagerClient:
        (std::unique_ptr<ios_web_view::WebViewPasswordManagerClient>)
            passwordManagerClient
       passwordController:(SharedPasswordController*)passwordController {
  // AutofillClientIOS registers itself on the WebState in its constructor and
  // CHECKs that only one is registered per WebState. Build the client exactly
  // once with |self| as the bridge and move it into the designated init as
  // |autofillClientForTest|, so the init reuses the same pointer instead of
  // creating a second client (which would trip the CHECK).
  return [self initWithWebState:webState
          autofillClientForTest:createAutofillClient.Run(webState, self)
                  autofillAgent:autofillAgent
                passwordManager:std::move(passwordManager)
          passwordManagerClient:std::move(passwordManagerClient)
             passwordController:passwordController];
}

@end

#pragma clang diagnostic pop
