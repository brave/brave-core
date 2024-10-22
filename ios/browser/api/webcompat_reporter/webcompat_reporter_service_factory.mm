// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/webcompat_reporter/webcompat_reporter_service_factory.h"

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"
#include "brave/ios/browser/api/webcompat_reporter/webcompat_reporter_service_delegate.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace webcompat_reporter {

// static
mojo::PendingRemote<mojom::WebcompatReporterHandler>
WebcompatReporterServiceFactory::GetHandlerForContext(
    ChromeBrowserState* browser_state) {
  auto* service = GetInstance()->GetServiceForBrowserState(browser_state, true);
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

std::unique_ptr<KeyedService>
WebcompatReporterServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  auto report_uploader = std::make_unique<WebcompatReportUploader>(
      context->GetSharedURLLoaderFactory());
  component_updater::ComponentUpdateService* cus =
      GetApplicationContext()->GetComponentUpdateService();
  return std::make_unique<WebcompatReporterService>(
      std::make_unique<WebcompatReporterServiceDelegateImpl>(cus),
      std::move(report_uploader));
}

}  // namespace webcompat_reporter
