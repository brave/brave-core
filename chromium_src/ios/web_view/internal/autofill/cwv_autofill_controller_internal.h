// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_CONTROLLER_INTERNAL_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_CONTROLLER_INTERNAL_H_

#include <ios/web_view/internal/autofill/cwv_autofill_controller_internal.h>  // IWYU pragma: export

using CreateAutofillClientCallback =
    base::RepeatingCallback<std::unique_ptr<autofill::WebViewAutofillClientIOS>(
        web::WebState*,
        id<CWVAutofillClientIOSBridge, AutofillDriverIOSBridge>)>;

// Expose a way to create an CWVAutofillController with a explicit
// WebViewAutofillClientIOS so we can avoid it making one with WebView factories
@interface CWVAutofillController (Internal)
- (instancetype)initWithWebState:(web::WebState*)webState
            createAutofillClient:
                (CreateAutofillClientCallback)createAutofillClientCallback
                   autofillAgent:(AutofillAgent*)autofillAgent
                 passwordManager:
                     (std::unique_ptr<password_manager::PasswordManager>)
                         passwordManager
           passwordManagerClient:
               (std::unique_ptr<ios_web_view::WebViewPasswordManagerClient>)
                   passwordManagerClient
              passwordController:(SharedPasswordController*)passwordController;
@end

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_CONTROLLER_INTERNAL_H_
