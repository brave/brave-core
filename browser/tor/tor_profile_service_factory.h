/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace tor {
class TorProfileService;
}  // namespace tor

// Singleton that owns all TorProfileService and associates them with
// Profiles.
class TorProfileServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  TorProfileServiceFactory(const TorProfileServiceFactory&) = delete;
  TorProfileServiceFactory& operator=(const TorProfileServiceFactory&) = delete;

  static tor::TorProfileService* GetForContext(
      content::BrowserContext* context);
  static TorProfileServiceFactory* GetInstance();

  static void SetTorDisabled(bool disabled);
  static bool IsTorDisabled();

 private:
  friend struct base::DefaultSingletonTraits<TorProfileServiceFactory>;

  static tor::TorProfileService* GetForContext(content::BrowserContext* context,
                                               bool create);

  TorProfileServiceFactory();
  ~TorProfileServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

#endif  // BRAVE_BROWSER_TOR_TOR_PROFILE_SERVICE_FACTORY_H_
