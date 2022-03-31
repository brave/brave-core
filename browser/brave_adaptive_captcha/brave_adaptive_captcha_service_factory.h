/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADAPTIVE_CAPTCHA_BRAVE_ADAPTIVE_CAPTCHA_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_ADAPTIVE_CAPTCHA_BRAVE_ADAPTIVE_CAPTCHA_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_delegate.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace brave_adaptive_captcha {

class BraveAdaptiveCaptchaService;

class BraveAdaptiveCaptchaServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static BraveAdaptiveCaptchaService* GetForProfile(Profile* profile);
  static BraveAdaptiveCaptchaServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      BraveAdaptiveCaptchaServiceFactory>;

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
