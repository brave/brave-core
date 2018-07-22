/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace payments {
class PaymentsService;
}

// Singleton that owns all PaymentsService and associates them with
// Profiles.
class PaymentsServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static payments::PaymentsService* GetForProfile(Profile* profile);

  static PaymentsServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<PaymentsServiceFactory>;

  PaymentsServiceFactory();
  ~PaymentsServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(PaymentsServiceFactory);
};

#endif  // BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_FACTORY_H_
