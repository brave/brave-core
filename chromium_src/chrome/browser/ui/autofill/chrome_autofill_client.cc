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
#include "components/autofill/core/browser/browser_autofill_manager.h"
#include "components/autofill/core/browser/form_import/form_data_importer.h"
#include "components/optimization_guide/core/optimization_guide_features.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/autofill_ai/chrome_autofill_ai_client.h"
#endif  // !BUILDFLAG(IS_ANDROID)

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

  AutofillOptimizationGuide* GetAutofillOptimizationGuide() const override {
    if (optimization_guide::features::IsOptimizationHintsEnabled()) {
      return ChromeAutofillClient::GetAutofillOptimizationGuide();
    }
    return nullptr;
  }

  bool IsAutocompleteEnabled() const override {
    auto enabled = ChromeAutofillClient::IsAutocompleteEnabled();
    if (!IsPrivateProfile(web_contents())) {
      return enabled;
    }
    enabled = enabled && GetPrefs()->GetBoolean(kBraveAutofillPrivateWindows);
    return enabled;
  }

  bool IsAutofillEnabled() const override {
    auto enabled = ChromeAutofillClient::IsAutofillEnabled();
    if (GetProfileType() != profile_metrics::BrowserProfileType::kIncognito &&
        GetProfileType() !=
            profile_metrics::BrowserProfileType::kOtherOffTheRecordProfile) {
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
