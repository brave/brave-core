/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/usermodel_service_factory.h"

#include "brave/components/brave_ads/browser/buildflags/buildflags.h"
#include "brave/components/brave_ads/browser/usermodel_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"


#include "user_model.h"
#include "usermodel_service.h"

namespace brave_ads {

// static
UsermodelService* UsermodelServiceFactory::GetForProfile(
    Profile* profile) {
  if (profile->IsOffTheRecord())
    return NULL;

  return static_cast<UsermodelService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
UsermodelServiceFactory* UsermodelServiceFactory::GetInstance() {
  return base::Singleton<UsermodelServiceFactory>::get();
}

UsermodelServiceFactory::UsermodelServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "UsermodelService",
          BrowserContextDependencyManager::GetInstance()) {
}

UsermodelServiceFactory::~UsermodelServiceFactory() {
}

KeyedService* UsermodelServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return NULL;
}

content::BrowserContext* UsermodelServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (context->IsOffTheRecord())
    return chrome::GetBrowserContextOwnInstanceInIncognito(context);

  // use original profile for session profiles
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool UsermodelServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace brave_ads
