/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SAFE_BROWSING_EXTENSION_TELEMETRY_EXTENSION_TELEMETRY_SERVICE_FACTORY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SAFE_BROWSING_EXTENSION_TELEMETRY_EXTENSION_TELEMETRY_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class Profile;

namespace safe_browsing {

class ExtensionTelemetryService;

class ExtensionTelemetryServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static ExtensionTelemetryService* GetForProfile(Profile* profile);

  // Get the singleton instance.
  static ExtensionTelemetryServiceFactory* GetInstance();

  ExtensionTelemetryServiceFactory(const ExtensionTelemetryServiceFactory&) =
      delete;
  ExtensionTelemetryServiceFactory& operator=(
      const ExtensionTelemetryServiceFactory&) = delete;

 private:
  friend class base::NoDestructor<ExtensionTelemetryServiceFactory>;

  ExtensionTelemetryServiceFactory();
  ~ExtensionTelemetryServiceFactory() override = default;

  // BrowserContextKeyedServiceFactory:
  bool ServiceIsCreatedWithBrowserContext() const override;
  bool ServiceIsNULLWhileTesting() const override;
};

}  // namespace safe_browsing

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SAFE_BROWSING_EXTENSION_TELEMETRY_EXTENSION_TELEMETRY_SERVICE_FACTORY_H_
