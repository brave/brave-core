// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_BRAVE_FARBLING_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_BRAVE_FARBLING_SERVICE_FACTORY_H_

#include <memory>

#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class Profile;
namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace brave {
class BraveFarblingService;

class BraveFarblingServiceFactory : public ProfileKeyedServiceFactory {
 public:
  BraveFarblingServiceFactory(const BraveFarblingServiceFactory&) = delete;
  BraveFarblingServiceFactory& operator=(const BraveFarblingServiceFactory&) =
      delete;

  static BraveFarblingServiceFactory* GetInstance();
  static BraveFarblingService* GetForProfile(Profile* profile);

 private:
  friend base::NoDestructor<BraveFarblingServiceFactory>;

  BraveFarblingServiceFactory();
  ~BraveFarblingServiceFactory() override;

  // ProfileKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};
}  // namespace brave

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_BRAVE_FARBLING_SERVICE_FACTORY_H_
