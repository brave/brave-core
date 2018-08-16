/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_service_factory.h"

#include "brave/browser/tor/tor_profile_service_impl.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"


// static
tor::TorProfileService* TorProfileServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<tor::TorProfileService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
TorProfileServiceFactory* TorProfileServiceFactory::GetInstance() {
  return base::Singleton<TorProfileServiceFactory>::get();
}

TorProfileServiceFactory::TorProfileServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "TorProfileService",
          BrowserContextDependencyManager::GetInstance()) {
}

TorProfileServiceFactory::~TorProfileServiceFactory() {
}

KeyedService* TorProfileServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  std::unique_ptr<tor::TorProfileService> tor_profile_service(
      new tor::TorProfileServiceImpl(Profile::FromBrowserContext(context)));
  return tor_profile_service.release();
}

content::BrowserContext* TorProfileServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (context->IsOffTheRecord())
    return chrome::GetBrowserContextOwnInstanceInIncognito(context);

  // use original profile for session profiles
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool TorProfileServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

void TorProfileServiceFactory::BrowserContextShutdown(
    content::BrowserContext* context) {
   BrowserContextKeyedServiceFactory::BrowserContextShutdown(context);
}
void TorProfileServiceFactory::BrowserContextDestroyed(
    content::BrowserContext* context) {
   BrowserContextKeyedServiceFactory::BrowserContextDestroyed(context);
}
