/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NTP_BACKGROUND_IMAGES_VIEW_COUNTER_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_NTP_BACKGROUND_IMAGES_VIEW_COUNTER_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace content {
class BrowserContext;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace ntp_background_images {

class ViewCounterService;

class ViewCounterServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  ViewCounterServiceFactory(const ViewCounterServiceFactory&) = delete;
  ViewCounterServiceFactory& operator=(const ViewCounterServiceFactory&) =
      delete;

  static ViewCounterService* GetForProfile(Profile* profile);
  static ViewCounterServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<ViewCounterServiceFactory>;

  ViewCounterServiceFactory();
  ~ViewCounterServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
};

}  // namespace ntp_background_images

#endif  // BRAVE_BROWSER_NTP_BACKGROUND_IMAGES_VIEW_COUNTER_SERVICE_FACTORY_H_
