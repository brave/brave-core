// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_CONTAINERS_CONTAINERS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_CONTAINERS_CONTAINERS_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class Profile;

namespace containers {
class ContainersService;
}

class ContainersServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static ContainersServiceFactory* GetInstance();
  static containers::ContainersService* GetForProfile(Profile* profile);

 private:
  friend base::NoDestructor<ContainersServiceFactory>;

  void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) final;
  bool ServiceIsCreatedWithBrowserContext() const override;

  ContainersServiceFactory();
  ~ContainersServiceFactory() override;

  ContainersServiceFactory(const ContainersServiceFactory&) = delete;
  ContainersServiceFactory& operator=(const ContainersServiceFactory&) = delete;

  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

#endif  // BRAVE_BROWSER_CONTAINERS_CONTAINERS_SERVICE_FACTORY_H_
