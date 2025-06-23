/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EMAIL_ALIASES_BROWSER_EMAIL_ALIASES_SERVICE_FACTORY_H_
#define BRAVE_COMPONENTS_EMAIL_ALIASES_BROWSER_EMAIL_ALIASES_SERVICE_FACTORY_H_

#include <memory>

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace email_aliases {

class EmailAliasesService;

class EmailAliasesServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static EmailAliasesService* GetForProfile(Profile* profile);
  static EmailAliasesServiceFactory* GetInstance();

  EmailAliasesServiceFactory();

 private:
  ~EmailAliasesServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace email_aliases

#endif  // BRAVE_COMPONENTS_EMAIL_ALIASES_BROWSER_EMAIL_ALIASES_SERVICE_FACTORY_H_
