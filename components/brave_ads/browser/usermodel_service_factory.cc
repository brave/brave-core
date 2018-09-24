/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/usermodel_service_factory.h"

#include "brave/components/brave_ads/browser/buildflags/buildflags.h"
#include "brave/components/brave_ads/browser/usermodel_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "base/bind.h"
#include "base/guid.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/i18n/time_formatting.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_runner_util.h"
#include "base/task/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "net/base/escape.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/url_util.h"
#include "net/url_request/url_fetcher.h"
#include "url/gurl.h"
#include "url/url_canon_stdstring.h"
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

  std::unique_ptr<UsermodelService> usermodel_service(
    new UsermodelService(Profile::FromBrowserContext(context))
  );

  return usermodel_service.release();
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
