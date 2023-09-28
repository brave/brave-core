// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/autofill/chrome_autofill_client.h"
#include "base/memory/ptr_util.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/autofill/payments/webauthn_dialog_controller_impl.h"
#include "chrome/browser/ui/page_info/page_info_dialog.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"

namespace autofill {

namespace {
bool IsPrivateProfile(content::WebContents* web_contents) {
  if (!web_contents) {
    return false;
  }
  auto* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  if (!profile) {
    return false;
  }
  return (profile_metrics::GetBrowserProfileType(profile) ==
          profile_metrics::BrowserProfileType::kIncognito) ||
         profile->IsTor();
}

}  // namespace

class BraveChromeAutofillClient : public ChromeAutofillClient {
 public:
  using ChromeAutofillClient::ChromeAutofillClient;

  bool IsAutocompleteEnabled() const override {
    auto enabled = ChromeAutofillClient::IsAutocompleteEnabled();
    if (!IsPrivateProfile(web_contents())) {
      return enabled;
    }
    enabled = enabled && GetPrefs()->GetBoolean(kBraveAutofillPrivateWindows);
    return enabled;
  }
};

}  // namespace autofill

#define WrapUnique WrapUnique(new autofill::BraveChromeAutofillClient(web_contents))); \
  if (0) std::unique_ptr<autofill::ChromeAutofillClient> dummy(
#include "src/chrome/browser/ui/autofill/chrome_autofill_client.cc"
#undef WrapUnique
