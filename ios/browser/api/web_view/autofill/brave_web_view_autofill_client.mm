// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/web_view/autofill/brave_web_view_autofill_client.h"

#include "components/application_locale_storage/application_locale_storage.h"
#include "ios/chrome/browser/autofill/model/autocomplete_history_manager_factory.h"
#include "ios/chrome/browser/autofill/model/autofill_log_router_factory.h"
#include "ios/chrome/browser/autofill/model/personal_data_manager_factory.h"
#include "ios/chrome/browser/autofill/model/strike_database_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_account_password_store_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_password_manager_client.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/signin/model/identity_manager_factory.h"
#include "ios/chrome/browser/sync/model/sync_service_factory.h"

namespace autofill {

// static
std::unique_ptr<WebViewAutofillClientIOS> BraveWebViewAutofillClientIOS::Create(
    web::WebState* web_state,
    id<CWVAutofillClientIOSBridge, AutofillDriverIOSBridge> bridge) {
  // Implemented similarily to WebViewAutofillClientIOS::Create but uses
  // Chrome's ProfileIOS
  ProfileIOS* profile =
      ProfileIOS::FromBrowserState(web_state->GetBrowserState());
  ProfileIOS* original_profile = profile->GetOriginalProfile();
  return static_cast<std::unique_ptr<WebViewAutofillClientIOS>>(
      std::make_unique<autofill::BraveWebViewAutofillClientIOS>(
          profile->GetPrefs(),
          autofill::PersonalDataManagerFactory::GetForProfile(original_profile),
          autofill::AutocompleteHistoryManagerFactory::GetForProfile(profile),
          web_state, bridge,
          IdentityManagerFactory::GetForProfile(original_profile),
          autofill::StrikeDatabaseFactory::GetForProfile(original_profile),
          SyncServiceFactory::GetForProfile(profile),
          autofill::AutofillLogRouterFactory::GetForProfile(profile)));
}

const std::string& BraveWebViewAutofillClientIOS::GetAppLocale() const {
  return GetApplicationContext()->GetApplicationLocaleStorage()->Get();
}

}  // namespace autofill
