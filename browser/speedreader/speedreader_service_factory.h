/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_SERVICE_FACTORY_H_

#include <memory>

#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
}

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace speedreader {

class SpeedreaderService;

class SpeedreaderServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static SpeedreaderServiceFactory* GetInstance();
  static SpeedreaderService* GetForBrowserContext(
      content::BrowserContext* browser_context);

 private:
  friend base::NoDestructor<SpeedreaderServiceFactory>;
  SpeedreaderServiceFactory();
  ~SpeedreaderServiceFactory() override;

  SpeedreaderServiceFactory(const SpeedreaderServiceFactory&) = delete;
  SpeedreaderServiceFactory& operator=(const SpeedreaderServiceFactory&) =
      delete;

  // BrowserContextKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace speedreader

#endif  // BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_SERVICE_FACTORY_H_
