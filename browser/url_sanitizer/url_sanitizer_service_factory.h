/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_URL_SANITIZER_URL_SANITIZER_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_URL_SANITIZER_URL_SANITIZER_SERVICE_FACTORY_H_

#include <memory>

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/components/url_sanitizer/common/mojom/url_sanitizer.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#endif  // # BUILDFLAG(IS_ANDROID)

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace brave {

class URLSanitizerService;

class URLSanitizerServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static URLSanitizerService* GetForBrowserContext(
      content::BrowserContext* context);
#if BUILDFLAG(IS_ANDROID)
  static mojo::PendingRemote<url_sanitizer::mojom::UrlSanitizerService>
  GetForContext(content::BrowserContext* context);
#endif  // # BUILDFLAG(IS_ANDROID)
  static URLSanitizerServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<URLSanitizerServiceFactory>;

  URLSanitizerServiceFactory();
  ~URLSanitizerServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
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
