/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGES_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGES_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class NTPSponsoredImagesServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static NTPSponsoredImagesServiceFactory* GetInstance();

  NTPSponsoredImagesServiceFactory(
      const NTPSponsoredImagesServiceFactory&) = delete;
  NTPSponsoredImagesServiceFactory& operator=(
      const NTPSponsoredImagesServiceFactory&) = delete;

 private:
  friend struct base::DefaultSingletonTraits<NTPSponsoredImagesServiceFactory>;

  NTPSponsoredImagesServiceFactory();
  ~NTPSponsoredImagesServiceFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

#endif  // BRAVE_BROWSER_NTP_SPONSORED_IMAGES_NTP_SPONSORED_IMAGES_SERVICE_FACTORY_H_

