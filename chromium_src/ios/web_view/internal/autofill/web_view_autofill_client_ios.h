// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_AUTOFILL_WEB_VIEW_AUTOFILL_CLIENT_IOS_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_AUTOFILL_WEB_VIEW_AUTOFILL_CLIENT_IOS_H_

#include "components/autofill/ios/browser/autofill_client_ios.h"
#include "ios/web_view/internal/autofill/ios_web_view_payments_autofill_client.h"

// This override replaces the IOSWebViewPaymentsAutofillClient with the
// BraveIOSWebViewPaymentsAutofillClient subclass and passes in the
// pref_service_ of the current WebViewAutofillClient into it
#define IOSWebViewPaymentsAutofillClient BraveIOSWebViewPaymentsAutofillClient
#define web_state() web_state(), pref_service_

#include <ios/web_view/internal/autofill/web_view_autofill_client_ios.h>  // IWYU pragma: export

#undef web_state
#undef IOSWebViewPaymentsAutofillClient

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_AUTOFILL_WEB_VIEW_AUTOFILL_CLIENT_IOS_H_
