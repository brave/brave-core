// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class Profile;

namespace traffic_control {
class TrafficControlService;
}

class TrafficControlServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static TrafficControlServiceFactory* GetInstance();
  static traffic_control::TrafficControlService* GetForProfile(
      Profile* profile);

 private:
  friend base::NoDestructor<TrafficControlServiceFactory>;

  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  bool ServiceIsCreatedWithBrowserContext() const override;

  TrafficControlServiceFactory();
  ~TrafficControlServiceFactory() override;

  TrafficControlServiceFactory(const TrafficControlServiceFactory&) = delete;
  TrafficControlServiceFactory& operator=(const TrafficControlServiceFactory&) =
      delete;

  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

#endif  // BRAVE_BROWSER_TRAFFIC_CONTROL_TRAFFIC_CONTROL_SERVICE_FACTORY_H_
