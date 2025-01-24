/* Copyright (c) 2015 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/signin/model/about_signin_internals_factory.h"

#include "base/no_destructor.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/signin/core/browser/about_signin_internals.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace ios {

AboutSigninInternalsFactory::AboutSigninInternalsFactory()
    : ProfileKeyedServiceFactoryIOS("AboutSigninInternals") {}

AboutSigninInternalsFactory::~AboutSigninInternalsFactory() = default;

// static
AboutSigninInternals* AboutSigninInternalsFactory::GetForProfile(
    ProfileIOS* profile) {
  return GetInstance()->GetServiceForProfileAs<AboutSigninInternals>(
      profile, /*create=*/true);
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
