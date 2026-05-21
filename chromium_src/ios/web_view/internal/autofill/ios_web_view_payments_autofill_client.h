// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_AUTOFILL_IOS_WEB_VIEW_PAYMENTS_AUTOFILL_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_AUTOFILL_IOS_WEB_VIEW_PAYMENTS_AUTOFILL_CLIENT_H_

// This override adds a subclass of IOSWebViewPaymentsAutofillClient which
// supports passing in a PrefService dependency directly rather than go through
// WebViewBrowserState which is unavailable for our usages of CWVWebView
#define client_ \
  client_;      \
  friend class BraveIOSWebViewPaymentsAutofillClient
#define GetPrefService \
  Unused();            \
  virtual PrefService* GetPrefService

#include <ios/web_view/internal/autofill/ios_web_view_payments_autofill_client.h>  // IWYU pragma: export

#undef GetPrefService
#undef client_

namespace autofill::payments {

class BraveIOSWebViewPaymentsAutofillClient
    : public IOSWebViewPaymentsAutofillClient {
 public:
  explicit BraveIOSWebViewPaymentsAutofillClient(
      autofill::WebViewAutofillClientIOS* client,
      id<CWVAutofillClientIOSBridge> bridge,
      web::WebState* web_state,
      PrefService* pref_service);

 private:
  raw_ptr<PrefService> pref_service_;
  PrefService* GetPrefService() const override;
};

}  // namespace autofill::payments

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_AUTOFILL_IOS_WEB_VIEW_PAYMENTS_AUTOFILL_CLIENT_H_
