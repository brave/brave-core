/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace speedreader {

class SpeedreaderService;

class SpeedreaderServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static SpeedreaderServiceFactory* GetInstance();
  static SpeedreaderService* GetForProfile(Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<SpeedreaderServiceFactory>;
  SpeedreaderServiceFactory();
  ~SpeedreaderServiceFactory() override;

  SpeedreaderServiceFactory(const SpeedreaderServiceFactory&) = delete;
  SpeedreaderServiceFactory& operator=(const SpeedreaderServiceFactory&) =
      delete;

  // BrowserContextKeyedServiceFactory overrides:

  // Speedreader works in OTR modes, but doesn't persists its pref changes
  // to the parent profile. So we override this to use OTR browser contexts
  // as-is.
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

}  // namespace speedreader

#endif  // BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_SERVICE_FACTORY_H_
