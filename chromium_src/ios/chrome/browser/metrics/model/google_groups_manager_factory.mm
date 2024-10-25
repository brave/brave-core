/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "ios/chrome/browser/metrics/model/google_groups_manager_factory.h"

#import "base/no_destructor.h"
#import "components/keyed_service/ios/browser_state_dependency_manager.h"
#import "components/variations/service/google_groups_manager.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"

// static
GoogleGroupsManager* GoogleGroupsManagerFactory::GetForProfile(
    ProfileIOS* profile) {
  return nullptr;
}

// static
GoogleGroupsManagerFactory*
GoogleGroupsManagerFactory::GetInstance() {
  static base::NoDestructor<GoogleGroupsManagerFactory> instance;
  return instance.get();
}

GoogleGroupsManagerFactory::GoogleGroupsManagerFactory()
    : BrowserStateKeyedServiceFactory(
          "GoogleGroupsManager",
          BrowserStateDependencyManager::GetInstance()) {}

std::unique_ptr<KeyedService>
GoogleGroupsManagerFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return nullptr;
}

bool GoogleGroupsManagerFactory::ServiceIsCreatedWithBrowserState()
    const {
  return true;
}

bool GoogleGroupsManagerFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

void GoogleGroupsManagerFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {}
