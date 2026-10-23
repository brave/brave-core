// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/webcompat_reporter/webcompat_reporter_service_factory.h"

#include <utility>

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"
#include "brave/components/webcompat_reporter/common/pref_names.h"
#include "brave/ios/browser/api/webcompat_reporter/webcompat_reporter_service_delegate.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace webcompat_reporter {

// static
mojo::PendingRemote<mojom::WebcompatReporterHandler>
WebcompatReporterServiceFactory::GetHandlerForContext(ProfileIOS* profile) {
  auto* service = GetForProfile(profile);
  if (!service) {
    return mojo::PendingRemote<mojom::WebcompatReporterHandler>();
  }
  return service->MakeRemote();
}

WebcompatReporterServiceFactory::WebcompatReporterServiceFactory(PassKey key)
    : TypedProfileKeyedServiceFactoryIOS(
          std::move(key),
          "WebcompatReporterService",
          ProfileSelection::kOwnInstanceInIncognito,
          ServiceCreation::kCreateLazily,
          TestingCreation::kCreateService) {}

void WebcompatReporterServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  webcompat_reporter::prefs::RegisterProfilePrefs(
      static_cast<PrefRegistrySimple*>(registry));
}

std::unique_ptr<KeyedService>
WebcompatReporterServiceFactory::BuildServiceInstanceFor(
    ProfileIOS* profile) const {
  auto report_uploader = std::make_unique<WebcompatReportUploader>(
      profile->GetSharedURLLoaderFactory());
  component_updater::ComponentUpdateService* cus =
      GetApplicationContext()->GetComponentUpdateService();
  return std::make_unique<WebcompatReporterService>(
      !profile || profile->IsOffTheRecord() ? nullptr : profile->GetPrefs(),
      std::make_unique<WebcompatReporterServiceDelegateImpl>(cus),
      std::move(report_uploader));
}

}  // namespace webcompat_reporter
