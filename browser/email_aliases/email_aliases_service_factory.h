/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

class KeyedService;
class Profile;

namespace email_aliases {

class EmailAliasesService;

class EmailAliasesServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static EmailAliasesService* GetServiceForProfile(Profile* profile);
  static void BindForProfile(
      Profile* profile,
      mojo::PendingReceiver<mojom::EmailAliasesService> receiver);
  static EmailAliasesServiceFactory* GetInstance();

  EmailAliasesServiceFactory();

 private:
  ~EmailAliasesServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace email_aliases

#endif  // BRAVE_BROWSER_EMAIL_ALIASES_EMAIL_ALIASES_SERVICE_FACTORY_H_
