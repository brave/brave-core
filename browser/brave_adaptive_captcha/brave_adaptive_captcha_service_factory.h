/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADAPTIVE_CAPTCHA_BRAVE_ADAPTIVE_CAPTCHA_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_ADAPTIVE_CAPTCHA_BRAVE_ADAPTIVE_CAPTCHA_SERVICE_FACTORY_H_

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "content/public/browser/browser_context.h"

class Profile;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace brave_adaptive_captcha {

class BraveAdaptiveCaptchaService;

class BraveAdaptiveCaptchaServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static BraveAdaptiveCaptchaService* GetForProfile(Profile* profile);
  static BraveAdaptiveCaptchaServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<BraveAdaptiveCaptchaServiceFactory>;

  BraveAdaptiveCaptchaServiceFactory();
  ~BraveAdaptiveCaptchaServiceFactory() override;

  BraveAdaptiveCaptchaServiceFactory(
      const BraveAdaptiveCaptchaServiceFactory&) = delete;
  BraveAdaptiveCaptchaServiceFactory& operator=(
      const BraveAdaptiveCaptchaServiceFactory&) = delete;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};

}  // namespace brave_adaptive_captcha

#endif  // BRAVE_BROWSER_BRAVE_ADAPTIVE_CAPTCHA_BRAVE_ADAPTIVE_CAPTCHA_SERVICE_FACTORY_H_
