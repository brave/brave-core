/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WORKSPACES_WORKSPACE_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_WORKSPACES_WORKSPACE_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

class WorkspaceService;
class Profile;

class WorkspaceServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static WorkspaceServiceFactory* GetInstance();
  static WorkspaceService* GetForProfile(Profile* profile);

  WorkspaceServiceFactory(const WorkspaceServiceFactory&) = delete;
  WorkspaceServiceFactory& operator=(const WorkspaceServiceFactory&) = delete;

 private:
  friend class base::NoDestructor<WorkspaceServiceFactory>;

  WorkspaceServiceFactory();
  ~WorkspaceServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

#endif  // BRAVE_BROWSER_WORKSPACES_WORKSPACE_SERVICE_FACTORY_H_
