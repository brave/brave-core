/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/debounce/debounce_service_factory.h"

#include "base/no_destructor.h"
#include "brave/components/debounce/core/browser/debounce_component_installer.h"
#include "brave/components/debounce/core/browser/debounce_service.h"
#include "brave/components/debounce/core/common/features.h"
#include "brave/ios/browser/api/debounce/debounce_service+private.h"
#include "brave/ios/browser/application_context/brave_application_context_impl.h"
#include "brave/ios/browser/debounce/debounce_service_factory+private.h"
#include "brave/ios/browser/keyed_service/keyed_service_factory_wrapper+private.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/browser_state.h"

@implementation DebounceServiceFactory
+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  // Create and start the local data file service and component installer
  debounce::DebounceService* debounceService =
      debounce::DebounceServiceFactory::GetServiceForState(profile);
  return [[DebounceService alloc] initWithDebounceService:debounceService];
}
@end

namespace debounce {

// static
debounce::DebounceService* DebounceServiceFactory::GetServiceForState(
    ProfileIOS* browser_state) {
  return static_cast<debounce::DebounceService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
DebounceServiceFactory* DebounceServiceFactory::GetInstance() {
  static base::NoDestructor<DebounceServiceFactory> instance;
  return instance.get();
}

DebounceServiceFactory::DebounceServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "DebounceService",
          BrowserStateDependencyManager::GetInstance()) {}

DebounceServiceFactory::~DebounceServiceFactory() = default;

std::unique_ptr<KeyedService> DebounceServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  if (!base::FeatureList::IsEnabled(debounce::features::kBraveDebounce)) {
    return nullptr;
  }

  BraveApplicationContextImpl* braveContext =
      static_cast<BraveApplicationContextImpl*>(GetApplicationContext());
  std::unique_ptr<debounce::DebounceService> service =
      std::make_unique<debounce::DebounceService>(
          braveContext->debounce_component_installer(),
          user_prefs::UserPrefs::Get(context));

  return service;
}

bool DebounceServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

web::BrowserState* DebounceServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace debounce
