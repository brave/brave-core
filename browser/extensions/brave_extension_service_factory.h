/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace extensions {

class BraveExtensionServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static BraveExtensionServiceFactory* GetInstance();

  BraveExtensionServiceFactory(const BraveExtensionServiceFactory&) = delete;
  BraveExtensionServiceFactory& operator=(const BraveExtensionServiceFactory&) =
      delete;

 private:
  friend base::NoDestructor<BraveExtensionServiceFactory>;

  BraveExtensionServiceFactory();
  ~BraveExtensionServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_SERVICE_FACTORY_H_
