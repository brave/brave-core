/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/metrics/model/google_groups_updater_service_factory.h"

#import "components/keyed_service/ios/browser_state_dependency_manager.h"
#import "components/variations/service/google_groups_updater_service.h"
#import "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"

// static
GoogleGroupsUpdaterService*
GoogleGroupsUpdaterServiceFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return nullptr;
}

// static
GoogleGroupsUpdaterServiceFactory*
GoogleGroupsUpdaterServiceFactory::GetInstance() {
  return nullptr;
}

GoogleGroupsUpdaterServiceFactory::GoogleGroupsUpdaterServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "GoogleGroupsUpdaterService",
          BrowserStateDependencyManager::GetInstance()) {}

std::unique_ptr<KeyedService>
GoogleGroupsUpdaterServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return nullptr;
}

bool GoogleGroupsUpdaterServiceFactory::ServiceIsCreatedWithBrowserState()
    const {
  return true;
}

bool GoogleGroupsUpdaterServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

void GoogleGroupsUpdaterServiceFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {}
