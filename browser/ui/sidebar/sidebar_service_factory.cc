/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "brave/components/sidebar/common/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/channel_info.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/channel.h"

namespace sidebar {

// static
SidebarServiceFactory* SidebarServiceFactory::GetInstance() {
  static base::NoDestructor<SidebarServiceFactory> instance;
  return instance.get();
}

SidebarService* SidebarServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<SidebarService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

SidebarServiceFactory::SidebarServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "SidebarService",
          BrowserContextDependencyManager::GetInstance()) {
  // Early return if the preference is already set or not existed(in test).
  if (!g_browser_process || !g_browser_process->local_state()) {
    return;
  }

  auto* preference = g_browser_process->local_state()->FindPreference(
      kTargetUserForSidebarEnabledTest);
  if (!preference) {
    return;
  }

  // Set target user(new user & feature enabled) flag only once.
  if (g_browser_process->local_state()->GetBoolean(
          kTargetUserForSidebarEnabledTest)) {
    return;
  }

  if (preference->IsDefaultValue()) {
    g_browser_process->local_state()->SetBoolean(
        kTargetUserForSidebarEnabledTest,
        base::FeatureList::IsEnabled(
            sidebar::features::kSidebarShowAlwaysOnStable) &&
            first_run::IsChromeFirstRun());
  }
}

SidebarServiceFactory::~SidebarServiceFactory() = default;

std::vector<SidebarItem::BuiltInItemType>
SidebarServiceFactory::GetBuiltInItemTypesForProfile(Profile* profile) const {
  std::vector<SidebarItem::BuiltInItemType> types;
  for (const auto& type : kDefaultBuiltInItemTypes) {
    if (profile->IsGuestSession()) {
      if (!IsDisabledItemForGuest(type)) {
        types.push_back(type);
      }
      continue;
    }

    // Private(or tor) profile.
    if (profile->IsIncognitoProfile()) {
      if (!IsDisabledItemForPrivate(type)) {
        types.push_back(type);
      }
      continue;
    }

    types.push_back(type);
  }

  return types;
}

std::unique_ptr<KeyedService>
SidebarServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto* profile = Profile::FromBrowserContext(context);
  return std::make_unique<SidebarService>(
      profile->GetPrefs(), GetBuiltInItemTypesForProfile(profile));
}

content::BrowserContext* SidebarServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // sidebar items list is not shared between normal and private windows.
  return GetBrowserContextOwnInstanceInIncognito(context);
}

void SidebarServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  SidebarService::RegisterProfilePrefs(
      registry, GetDefaultShowSidebarOption(chrome::GetChannel()));
}

}  // namespace sidebar
