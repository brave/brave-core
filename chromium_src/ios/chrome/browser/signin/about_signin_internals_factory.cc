// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/signin/about_signin_internals_factory.h"

#include "base/no_destructor.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/signin/core/browser/about_signin_internals.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"

namespace ios {

AboutSigninInternalsFactory::AboutSigninInternalsFactory()
    : BrowserStateKeyedServiceFactory(
          "AboutSigninInternals",
          BrowserStateDependencyManager::GetInstance()) {}

AboutSigninInternalsFactory::~AboutSigninInternalsFactory() {}

// static
AboutSigninInternals* AboutSigninInternalsFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<AboutSigninInternals*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
AboutSigninInternalsFactory* AboutSigninInternalsFactory::GetInstance() {
  static base::NoDestructor<AboutSigninInternalsFactory> instance;
  return instance.get();
}

std::unique_ptr<KeyedService>
AboutSigninInternalsFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return nullptr;
}

void AboutSigninInternalsFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* user_prefs) {}

}  // namespace ios
