/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_REDUCE_LANGUAGE_REDUCE_LANGUAGE_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_REDUCE_LANGUAGE_REDUCE_LANGUAGE_SERVICE_FACTORY_H_

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace reduce_language {

class ReduceLanguageService;

class ReduceLanguageServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static ReduceLanguageService* GetForBrowserContext(
      content::BrowserContext* context);
  static ReduceLanguageServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<ReduceLanguageServiceFactory>;

  ReduceLanguageServiceFactory();
  ~ReduceLanguageServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  ReduceLanguageServiceFactory(const ReduceLanguageServiceFactory&) = delete;
  ReduceLanguageServiceFactory& operator=(const ReduceLanguageServiceFactory&) =
      delete;
};

}  // namespace reduce_language

#endif  // BRAVE_BROWSER_REDUCE_LANGUAGE_REDUCE_LANGUAGE_SERVICE_FACTORY_H_
