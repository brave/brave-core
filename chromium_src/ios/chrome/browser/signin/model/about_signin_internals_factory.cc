/* Copyright (c) 2015 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/signin/model/about_signin_internals_factory.h"

#include <utility>

#include "components/pref_registry/pref_registry_syncable.h"
#include "components/signin/core/browser/about_signin_internals.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace ios {

AboutSigninInternalsFactory::AboutSigninInternalsFactory(PassKey key)
    : TypedProfileKeyedServiceFactoryIOS(std::move(key),
                                         "AboutSigninInternals") {}

std::unique_ptr<KeyedService>
AboutSigninInternalsFactory::BuildServiceInstanceFor(
    ProfileIOS* profile) const {
  return nullptr;
}

void AboutSigninInternalsFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* user_prefs) {}

}  // namespace ios
