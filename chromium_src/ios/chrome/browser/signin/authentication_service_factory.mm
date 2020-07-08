// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/signin/authentication_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/signin/authentication_service.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"


#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

std::unique_ptr<KeyedService> BuildAuthenticationService(
    web::BrowserState* context) {
  return std::unique_ptr<AuthenticationService>();
}

}  // namespace

// static
AuthenticationService* AuthenticationServiceFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  AuthenticationService* service = static_cast<AuthenticationService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
  return service;
}

// static
AuthenticationServiceFactory* AuthenticationServiceFactory::GetInstance() {
  static base::NoDestructor<AuthenticationServiceFactory> instance;
  return instance.get();
}

// static
void AuthenticationServiceFactory::CreateAndInitializeForBrowserState(
    ChromeBrowserState* browser_state,
    std::unique_ptr<AuthenticationServiceDelegate> delegate) {}

// static
AuthenticationServiceFactory::TestingFactory
AuthenticationServiceFactory::GetDefaultFactory() {
  return base::BindRepeating(&BuildAuthenticationService);
}

AuthenticationServiceFactory::AuthenticationServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "AuthenticationService",
          BrowserStateDependencyManager::GetInstance()) {}

AuthenticationServiceFactory::~AuthenticationServiceFactory() {}

std::unique_ptr<KeyedService>
AuthenticationServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return BuildAuthenticationService(context);
}

void AuthenticationServiceFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {}

bool AuthenticationServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
