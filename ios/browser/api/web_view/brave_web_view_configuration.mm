// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/web_view/brave_web_view_configuration.h"

#include "brave/ios/browser/ui/web_view/features.h"
#include "components/autofill/core/browser/data_manager/personal_data_manager.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/password_store/password_store_interface.h"
#include "ios/chrome/browser/affiliations/model/ios_chrome_affiliation_service_factory.h"
#include "ios/chrome/browser/autofill/model/personal_data_manager_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_account_password_store_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web_view/internal/autofill/cwv_autofill_data_manager_internal.h"
#include "ios/web_view/internal/cwv_web_view_configuration_internal.h"

@implementation BraveWebViewConfiguration {
  CWVAutofillDataManager* _autofillDataManager;
}

- (CWVAutofillDataManager*)autofillDataManager {
  // Reimplements CWVWebViewConfiguration's `autofillDataManager` method to
  // instead create a CWVAutofillDataManager using Chrome factories instead
  // of `//ios/web_view` specific factories.
  if (!base::FeatureList::IsEnabled(
          brave::features::kUseChromiumWebViewsAutofill)) {
    return nil;
  }
  if (!_autofillDataManager && self.persistent) {
    ProfileIOS* profile = ProfileIOS::FromBrowserState(self.browserState);
    autofill::PersonalDataManager* personalDataManager =
        autofill::PersonalDataManagerFactory::GetForProfile(profile);
    scoped_refptr<password_manager::PasswordStoreInterface> passwordStore =
        IOSChromeAccountPasswordStoreFactory::GetForProfile(
            profile, ServiceAccessType::EXPLICIT_ACCESS);
    affiliations::AffiliationService* affiliation_service =
        IOSChromeAffiliationServiceFactory::GetForProfile(profile);
    _autofillDataManager = [[CWVAutofillDataManager alloc]
         initWithPersonalDataManager:personalDataManager
                       passwordStore:passwordStore.get()
                 affiliationsService:affiliation_service
        isPasswordAffiliationEnabled:NO];
  }
  return _autofillDataManager;
}

- (void)shutDown {
  [_autofillDataManager shutDown];
  [super shutDown];
}

@end
