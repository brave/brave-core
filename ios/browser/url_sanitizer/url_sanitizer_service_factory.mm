/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/url_sanitizer/url_sanitizer_service_factory.h"

#include "base/no_destructor.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_component_installer.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_service.h"
#include "brave/ios/browser/api/url_sanitizer/url_sanitizer_service+private.h"
#include "brave/ios/browser/application_context/brave_application_context_impl.h"
#include "brave/ios/browser/keyed_service/keyed_service_factory_wrapper+private.h"
#include "brave/ios/browser/url_sanitizer/url_sanitizer_service_factory+private.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/browser_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation URLSanitizerServiceFactory
+ (nullable id)serviceForBrowserState:(ProfileIOS*)profile {
  // Create and start the local data file service and component installer
  brave::URLSanitizerService* urlSanitizer =
      brave::URLSanitizerServiceFactory::GetServiceForState(profile);
  return [[URLSanitizerService alloc] initWithURLSanitizerService:urlSanitizer];
}
@end

namespace brave {

// static
brave::URLSanitizerService* URLSanitizerServiceFactory::GetServiceForState(
    ProfileIOS* profile) {
  return static_cast<brave::URLSanitizerService*>(
      GetInstance()->GetServiceForBrowserState(profile, true));
}

// static
URLSanitizerServiceFactory* URLSanitizerServiceFactory::GetInstance() {
  static base::NoDestructor<URLSanitizerServiceFactory> instance;
  return instance.get();
}

URLSanitizerServiceFactory::URLSanitizerServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "URLSanitizerService",
          BrowserStateDependencyManager::GetInstance()) {}

URLSanitizerServiceFactory::~URLSanitizerServiceFactory() = default;

std::unique_ptr<KeyedService>
URLSanitizerServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  std::unique_ptr<brave::URLSanitizerService> service =
      std::make_unique<brave::URLSanitizerService>();
  BraveApplicationContextImpl* braveContext =
      static_cast<BraveApplicationContextImpl*>(GetApplicationContext());
  braveContext->url_sanitizer_component_installer()->AddObserver(service.get());
  return service;
}

bool URLSanitizerServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

bool URLSanitizerServiceFactory::ServiceIsCreatedWithBrowserState() const {
  return true;
}

web::BrowserState* URLSanitizerServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace brave
