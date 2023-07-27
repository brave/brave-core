/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_PRIVACY_URL_SANITIZER_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_BRAVE_PRIVACY_URL_SANITIZER_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/url_sanitizer/browser/url_sanitizer_service.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace web {
class BrowserState;
}  // namespace web

class KeyedService;
class ChromeBrowserState;

namespace brave_privacy {

class URLSanitizerServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  static brave::URLSanitizerService* GetServiceForState(
      ChromeBrowserState* browser_state);

  static URLSanitizerServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<URLSanitizerServiceFactory>;

  URLSanitizerServiceFactory();
  ~URLSanitizerServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;
};

}  // namespace brave_privacy

#endif  // BRAVE_IOS_BROWSER_BRAVE_PRIVACY_URL_SANITIZER_SERVICE_FACTORY_H_
