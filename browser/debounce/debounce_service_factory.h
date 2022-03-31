/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_DEBOUNCE_DEBOUNCE_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_DEBOUNCE_DEBOUNCE_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace debounce {

class DebounceService;

class DebounceServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static DebounceService* GetForBrowserContext(
      content::BrowserContext* context);
  static DebounceServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<DebounceServiceFactory>;

  DebounceServiceFactory();
  ~DebounceServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DebounceServiceFactory(const DebounceServiceFactory&) = delete;
  DebounceServiceFactory& operator=(const DebounceServiceFactory&) = delete;
};

}  // namespace debounce

#endif  // BRAVE_BROWSER_DEBOUNCE_DEBOUNCE_SERVICE_FACTORY_H_
