/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NTP_BACKGROUND_BRAVE_NTP_CUSTOM_BACKGROUND_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_NTP_BACKGROUND_BRAVE_NTP_CUSTOM_BACKGROUND_SERVICE_FACTORY_H_

#include <memory>

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

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
  friend base::NoDestructor<BraveNTPCustomBackgroundServiceFactory>;

  BraveNTPCustomBackgroundServiceFactory();
  ~BraveNTPCustomBackgroundServiceFactory() override;

  BraveNTPCustomBackgroundServiceFactory(
      const BraveNTPCustomBackgroundServiceFactory&) = delete;
  BraveNTPCustomBackgroundServiceFactory& operator=(
      const BraveNTPCustomBackgroundServiceFactory&) = delete;

  // BrowserContextKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

#endif  // BRAVE_BROWSER_NTP_BACKGROUND_BRAVE_NTP_CUSTOM_BACKGROUND_SERVICE_FACTORY_H_
