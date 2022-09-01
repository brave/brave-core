/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_URL_SANITIZER_URL_SANITIZER_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_URL_SANITIZER_URL_SANITIZER_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace brave {

class URLSanitizerService;

class URLSanitizerServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static URLSanitizerService* GetForBrowserContext(
      content::BrowserContext* context);
  static URLSanitizerServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<URLSanitizerServiceFactory>;

  URLSanitizerServiceFactory();
  ~URLSanitizerServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  URLSanitizerServiceFactory(const URLSanitizerServiceFactory&) = delete;
  URLSanitizerServiceFactory& operator=(const URLSanitizerServiceFactory&) =
      delete;
};

}  // namespace brave

#endif  // BRAVE_BROWSER_URL_SANITIZER_URL_SANITIZER_SERVICE_FACTORY_H_
