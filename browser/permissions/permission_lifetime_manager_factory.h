/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PERMISSIONS_PERMISSION_LIFETIME_MANAGER_FACTORY_H_
#define BRAVE_BROWSER_PERMISSIONS_PERMISSION_LIFETIME_MANAGER_FACTORY_H_

#include <memory>

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace content {
class BrowserContext;
}

namespace permissions {
class PermissionLifetimeManager;
}

class Profile;

class PermissionLifetimeManagerFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static permissions::PermissionLifetimeManager* GetForProfile(
      content::BrowserContext* profile);
  static PermissionLifetimeManagerFactory* GetInstance();

 private:
  friend base::NoDestructor<PermissionLifetimeManagerFactory>;

  PermissionLifetimeManagerFactory();
  PermissionLifetimeManagerFactory(const PermissionLifetimeManagerFactory&) =
      delete;
  PermissionLifetimeManagerFactory& operator=(
      const PermissionLifetimeManagerFactory&) = delete;
  ~PermissionLifetimeManagerFactory() override;

  // BrowserContextKeyedServiceFactory methods:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
};

#endif  // BRAVE_BROWSER_PERMISSIONS_PERMISSION_LIFETIME_MANAGER_FACTORY_H_
