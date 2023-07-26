/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_URL_SANITIZER_URL_SANITIZER_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_URL_SANITIZER_URL_SANITIZER_SERVICE_FACTORY_H_

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace ios {

class URLSanitizerService;

class URLSanitizerServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static URLSanitizerService* GetForBrowserContext(
      content::BrowserContext* context);
  static URLSanitizerServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<URLSanitizerServiceFactory>;

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

}  // namespace ios

#endif  // BRAVE_IOS_BROWSER_URL_SANITIZER_URL_SANITIZER_SERVICE_FACTORY_H_