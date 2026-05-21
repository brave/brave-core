// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <ios/web_view/internal/autofill/ios_web_view_payments_autofill_client.mm>

namespace autofill::payments {

BraveIOSWebViewPaymentsAutofillClient::BraveIOSWebViewPaymentsAutofillClient(
    autofill::WebViewAutofillClientIOS* client,
    id<CWVAutofillClientIOSBridge> bridge,
    web::WebState* web_state,
    PrefService* pref_service)
    : IOSWebViewPaymentsAutofillClient(client, bridge, web_state),
      pref_service_(pref_service) {}

PrefService* BraveIOSWebViewPaymentsAutofillClient::GetPrefService() const {
  return pref_service_;
}

}  // namespace autofill::payments
