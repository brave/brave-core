/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WORKSPACE_BRAVE_WORKSPACE_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_WORKSPACE_BRAVE_WORKSPACE_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/pref_registry/pref_registry_syncable.h"

class BraveWorkspaceService;
class Profile;

class BraveWorkspaceServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static BraveWorkspaceServiceFactory* GetInstance();
  static BraveWorkspaceService* GetForProfile(Profile* profile);

  BraveWorkspaceServiceFactory(const BraveWorkspaceServiceFactory&) = delete;
  BraveWorkspaceServiceFactory& operator=(const BraveWorkspaceServiceFactory&) =
      delete;

 private:
  friend class base::NoDestructor<BraveWorkspaceServiceFactory>;

  BraveWorkspaceServiceFactory();
  ~BraveWorkspaceServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

#endif  // BRAVE_BROWSER_WORKSPACE_BRAVE_WORKSPACE_SERVICE_FACTORY_H_
