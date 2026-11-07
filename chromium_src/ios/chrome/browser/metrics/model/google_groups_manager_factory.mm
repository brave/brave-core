/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "ios/chrome/browser/metrics/model/google_groups_manager_factory.h"

#import "components/variations/service/google_groups_manager.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"

GoogleGroupsManagerFactory::GoogleGroupsManagerFactory(PassKey key)
    : TypedProfileKeyedServiceFactoryIOS(std::move(key),
                                         "GoogleGroupsManager",
                                         ServiceCreation::kCreateWithProfile,
                                         TestingCreation::kNoServiceForTests) {}

std::unique_ptr<KeyedService>
GoogleGroupsManagerFactory::BuildServiceInstanceFor(ProfileIOS* profile) const {
  return nullptr;
}

void GoogleGroupsManagerFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {}
