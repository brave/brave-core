// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/brave_shields/brave_shields_service_factory.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace brave_shields {

// static
mojo::PendingRemote<mojom::BraveShieldsUtilsService>
BraveShieldsUtilsServiceFactory::GetHandlerForContext(ProfileIOS* profile) {
  auto* service =
      GetInstance()->GetServiceForProfileAs<BraveShieldsService>(profile, true);
  if (!service) {
    return mojo::PendingRemote<mojom::BraveShieldsUtilsService>();
  }
  return service->MakeRemote();
}

// static
BraveShieldsUtilsServiceFactory*
BraveShieldsUtilsServiceFactory::GetInstance() {
  static base::NoDestructor<BraveShieldsUtilsServiceFactory> instance;
  return instance.get();
}

BraveShieldsUtilsServiceFactory::BraveShieldsUtilsServiceFactory()
    : ProfileKeyedServiceFactoryIOS("BraveShieldsUtilsService",
                                    ProfileSelection::kOwnInstanceInIncognito,
                                    ServiceCreation::kCreateLazily,
                                    TestingCreation::kCreateService) {}

BraveShieldsUtilsServiceFactory::~BraveShieldsUtilsServiceFactory() {}

void BraveShieldsUtilsServiceFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  brave_shields::prefs::RegisterProfilePrefs(
      static_cast<PrefRegistrySimple*>(registry));
}

std::unique_ptr<KeyedService>
BraveShieldsUtilsServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* profile = ProfileIOS::FromBrowserState(context);
  auto* map = ios::HostContentSettingsMapFactory::GetForProfile(profile);
  auto* localState = GetApplicationContext()->GetLocalState();
  auto* profilePrefs = isPrivate
                           ? _profile->GetOffTheRecordProfile()->GetPrefs()
                           : _profile->GetPrefs();
  return std::make_unique<BraveShieldsService>(map, localState, profilePrefs);
}

}  // namespace brave_shields
