// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/webcompat_reporter/webcompat_reporter_service_factory.h"

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"
#include "brave/components/webcompat_reporter/common/pref_names.h"
#include "brave/ios/browser/api/webcompat_reporter/webcompat_reporter_service_delegate.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace webcompat_reporter {

// static
mojo::PendingRemote<mojom::WebcompatReporterHandler>
WebcompatReporterServiceFactory::GetHandlerForContext(ProfileIOS* profile) {
  auto* service = GetInstance()->GetServiceForBrowserState(profile, true);
  if (!service) {
    return mojo::PendingRemote<mojom::WebcompatReporterHandler>();
  }
  return static_cast<WebcompatReporterService*>(service)->MakeRemote();
}

// static
WebcompatReporterServiceFactory*
WebcompatReporterServiceFactory::GetInstance() {
  static base::NoDestructor<WebcompatReporterServiceFactory> instance;
  return instance.get();
}

WebcompatReporterServiceFactory::WebcompatReporterServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "WebcompatReporterService",
          BrowserStateDependencyManager::GetInstance()) {}

WebcompatReporterServiceFactory::~WebcompatReporterServiceFactory() {}

void WebcompatReporterServiceFactory::RegisterBrowserStatePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  webcompat_reporter::prefs::RegisterProfilePrefs(
      static_cast<PrefRegistrySimple*>(registry));
}

std::unique_ptr<KeyedService>
WebcompatReporterServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto* profile = ProfileIOS::FromBrowserState(context);
  auto report_uploader = std::make_unique<WebcompatReportUploader>(
      context->GetSharedURLLoaderFactory());
  component_updater::ComponentUpdateService* cus =
      GetApplicationContext()->GetComponentUpdateService();
  return std::make_unique<WebcompatReporterService>(
      !profile || profile->IsOffTheRecord() ? nullptr : profile->GetPrefs(),
      std::make_unique<WebcompatReporterServiceDelegateImpl>(cus),
      std::move(report_uploader));
}

web::BrowserState* WebcompatReporterServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateOwnInstanceInIncognito(context);
}

}  // namespace webcompat_reporter
