/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/permissions/brave_permission_manager_factory.h"

#include "brave/browser/permissions/brave_permission_manager.h"
#include "chrome/browser/permissions/permission_manager.h"
#include "chrome/browser/permissions/permission_manager.h"
#include "chrome/browser/profiles/profile.h"

// static
PermissionManager*
BravePermissionManagerFactory::GetForProfile(Profile* profile) {
  return static_cast<PermissionManager*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
BravePermissionManagerFactory* BravePermissionManagerFactory::GetInstance() {
  return base::Singleton<BravePermissionManagerFactory>::get();
}

BravePermissionManagerFactory::BravePermissionManagerFactory()
    : PermissionManagerFactory() {
}

BravePermissionManagerFactory::~BravePermissionManagerFactory() {
}

KeyedService* BravePermissionManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BravePermissionManager(Profile::FromBrowserContext(context));
}
