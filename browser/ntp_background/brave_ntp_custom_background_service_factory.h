/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NTP_BACKGROUND_NTP_CUSTOM_BACKGROUND_IMAGES_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_NTP_BACKGROUND_NTP_CUSTOM_BACKGROUND_IMAGES_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace ntp_background_images {
class BraveNTPCustomBackgroundService;
}

class BraveNTPCustomBackgroundServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static ntp_background_images::BraveNTPCustomBackgroundService* GetForContext(
      content::BrowserContext* context);
  static BraveNTPCustomBackgroundServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      BraveNTPCustomBackgroundServiceFactory>;

  BraveNTPCustomBackgroundServiceFactory();
  ~BraveNTPCustomBackgroundServiceFactory() override;

  BraveNTPCustomBackgroundServiceFactory(
      const BraveNTPCustomBackgroundServiceFactory&) = delete;
  BraveNTPCustomBackgroundServiceFactory& operator=(
      const BraveNTPCustomBackgroundServiceFactory&) = delete;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};

#endif  // BRAVE_BROWSER_NTP_BACKGROUND_NTP_CUSTOM_BACKGROUND_IMAGES_SERVICE_FACTORY_H_
