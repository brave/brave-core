// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_LOCAL_AI_CANDLE_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_LOCAL_AI_CANDLE_SERVICE_FACTORY_H_

#include <memory>

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
}

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace local_ai {

class CandleService;

class CandleServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static CandleServiceFactory* GetInstance();
  static CandleService* GetForBrowserContext(
      content::BrowserContext* browser_context);

 private:
  friend base::NoDestructor<CandleServiceFactory>;
  CandleServiceFactory();
  ~CandleServiceFactory() override;

  CandleServiceFactory(const CandleServiceFactory&) = delete;
  CandleServiceFactory& operator=(const CandleServiceFactory&) = delete;

  // BrowserContextKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace local_ai

#endif  // BRAVE_BROWSER_LOCAL_AI_CANDLE_SERVICE_FACTORY_H_
