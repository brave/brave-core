// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_LOCAL_AI_CANDLE_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_LOCAL_AI_CANDLE_SERVICE_FACTORY_H_

#include <memory>

#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class KeyedService;
class Profile;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace local_ai {

class CandleService;

class CandleServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static CandleServiceFactory* GetInstance();
  static CandleService* GetForProfile(Profile* profile);

  CandleServiceFactory();

 private:
  friend base::NoDestructor<CandleServiceFactory>;
  ~CandleServiceFactory() override;

  CandleServiceFactory(const CandleServiceFactory&) = delete;
  CandleServiceFactory& operator=(const CandleServiceFactory&) = delete;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace local_ai

#endif  // BRAVE_BROWSER_LOCAL_AI_CANDLE_SERVICE_FACTORY_H_
