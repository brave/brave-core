/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_REQUEST_OTR_REQUEST_OTR_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_REQUEST_OTR_REQUEST_OTR_SERVICE_FACTORY_H_

#include <memory>

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace request_otr {

class RequestOTRService;

class RequestOTRServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static RequestOTRService* GetForBrowserContext(
      content::BrowserContext* context);
  static RequestOTRServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<RequestOTRServiceFactory>;

  RequestOTRServiceFactory();
  ~RequestOTRServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  RequestOTRServiceFactory(const RequestOTRServiceFactory&) = delete;
  RequestOTRServiceFactory& operator=(const RequestOTRServiceFactory&) = delete;
};

}  // namespace request_otr

#endif  // BRAVE_BROWSER_REQUEST_OTR_REQUEST_OTR_SERVICE_FACTORY_H_
