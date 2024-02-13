/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_service_factory.h"

#include "base/no_destructor.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

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
          BrowserContextDependencyManager::GetInstance()) {}

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

KeyedService* SidebarServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* profile = Profile::FromBrowserContext(context);
  return new SidebarService(profile->GetPrefs(),
                            GetBuiltInItemTypesForProfile(profile));
}

content::BrowserContext* SidebarServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // sidebar items list is not shared between normal and private windows.
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}

}  // namespace sidebar
