/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/payments/payments_service_factory.h"

#include "brave/browser/payments/payments_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

#if defined(BRAVE_PAYMENTS_ENABLED)
#include "brave/browser/payments/payments_service_impl.h"
#endif

// static
payments::PaymentsService* PaymentsServiceFactory::GetForProfile(
    Profile* profile) {
  if (profile->IsOffTheRecord())
    return NULL;

  return static_cast<payments::PaymentsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
PaymentsServiceFactory* PaymentsServiceFactory::GetInstance() {
  return base::Singleton<PaymentsServiceFactory>::get();
}

PaymentsServiceFactory::PaymentsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "PaymentsService",
          BrowserContextDependencyManager::GetInstance()) {
}

PaymentsServiceFactory::~PaymentsServiceFactory() {
}

KeyedService* PaymentsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
#if defined(BRAVE_PAYMENTS_ENABLED)
  std::unique_ptr<payments::PaymentsService> payments_service(
      new payments::PaymentsServiceImpl());
  return payments_service.release();
#else
  return NULL;
#endif
}

content::BrowserContext* PaymentsServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (context->IsOffTheRecord())
    return chrome::GetBrowserContextOwnInstanceInIncognito(context);

  // use original profile for session profiles
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool PaymentsServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
