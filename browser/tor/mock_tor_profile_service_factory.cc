/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/mock_tor_profile_service_factory.h"

#include <set>

#include "brave/browser/tor/mock_tor_profile_service_impl.h"
#include "brave/browser/renderer_host/brave_navigation_ui_data.h"
#include "brave/common/tor/pref_names.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"

// static
tor::TorProfileService* MockTorProfileServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<tor::TorProfileService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
MockTorProfileServiceFactory* MockTorProfileServiceFactory::GetInstance() {
  return base::Singleton<MockTorProfileServiceFactory>::get();
}

MockTorProfileServiceFactory::MockTorProfileServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "MockTorProfileService",
          BrowserContextDependencyManager::GetInstance()) {
}

//static
void MockTorProfileServiceFactory::SetTorNavigationUIData(
    Profile* profile, BraveNavigationUIData* data) {
  if (!profile->IsTorProfile())
    return;
  data->SetTorProfileService(GetForProfile(profile));
}

MockTorProfileServiceFactory::~MockTorProfileServiceFactory() {}

KeyedService* MockTorProfileServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  std::unique_ptr<tor::TorProfileService> tor_profile_service(
      new tor::MockTorProfileServiceImpl(profile));
  return tor_profile_service.release();
}

content::BrowserContext* MockTorProfileServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (context->IsOffTheRecord())
    return chrome::GetBrowserContextOwnInstanceInIncognito(context);

  // use original profile for session profiles
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
