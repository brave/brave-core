/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_DE_AMP_DE_AMP_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_DE_AMP_DE_AMP_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace de_amp {

class DeAmpService;

class DeAmpServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static DeAmpService* GetForBrowserContext(content::BrowserContext* context);
  static DeAmpServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<DeAmpServiceFactory>;

  DeAmpServiceFactory();
  ~DeAmpServiceFactory() override;
  DeAmpServiceFactory(const DeAmpServiceFactory&) = delete;
  DeAmpServiceFactory& operator=(const DeAmpServiceFactory&) = delete;

  // BrowserContextKeyedServiceFactory overrides:
  // DeAmp works in incognito mode, but shouldn't persist pref changes
  // to the parent profile.
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
};

}  // namespace de_amp

#endif  // BRAVE_BROWSER_DE_AMP_DE_AMP_SERVICE_FACTORY_H_
