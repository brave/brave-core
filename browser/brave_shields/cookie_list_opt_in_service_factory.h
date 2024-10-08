/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_COOKIE_LIST_OPT_IN_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_COOKIE_LIST_OPT_IN_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/brave_shields/core/common/cookie_list_opt_in.mojom.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace brave_shields {

class CookieListOptInService;

class CookieListOptInServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  CookieListOptInServiceFactory(const CookieListOptInServiceFactory&) = delete;
  CookieListOptInServiceFactory& operator=(
      const CookieListOptInServiceFactory&) = delete;

  static mojo::PendingRemote<mojom::CookieListOptInPageAndroidHandler>
  GetForContext(content::BrowserContext* context);
  static CookieListOptInService* GetServiceForContext(
      content::BrowserContext* context);
  static CookieListOptInServiceFactory* GetInstance();
  static void BindForContext(
      content::BrowserContext* context,
      mojo::PendingReceiver<mojom::CookieListOptInPageAndroidHandler> receiver);

 private:
  friend base::NoDestructor<CookieListOptInServiceFactory>;

  CookieListOptInServiceFactory();
  ~CookieListOptInServiceFactory() override;

  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_COOKIE_LIST_OPT_IN_SERVICE_FACTORY_H_
