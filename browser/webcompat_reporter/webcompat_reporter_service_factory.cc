/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/webcompat_reporter/webcompat_reporter_service_factory.h"

#include <memory>
#include <utility>

#include "base/no_destructor.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/webcompat_reporter/webcompat_reporter_service_delegate.h"
#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/storage_partition.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"

namespace webcompat_reporter {

// static
WebcompatReporterServiceFactory*
WebcompatReporterServiceFactory::GetInstance() {
  static base::NoDestructor<WebcompatReporterServiceFactory> instance;
  return instance.get();
}

// static
mojo::PendingRemote<mojom::WebcompatReporterHandler>
WebcompatReporterServiceFactory::GetHandlerForContext(
    content::BrowserContext* context) {
  return static_cast<WebcompatReporterService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// static
WebcompatReporterService* WebcompatReporterServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  return static_cast<WebcompatReporterService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

WebcompatReporterServiceFactory::WebcompatReporterServiceFactory()
    : ProfileKeyedServiceFactory(
          "WebcompatReporterService",
          ProfileSelections::BuildForRegularAndIncognito()) {}

WebcompatReporterServiceFactory::~WebcompatReporterServiceFactory() = default;

std::unique_ptr<KeyedService>
WebcompatReporterServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  DCHECK(context);
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  if (!default_storage_partition) {
    return nullptr;
  }
  auto* profile = Profile::FromBrowserContext(context);
  auto* prefs =
      !profile || profile->IsOffTheRecord() ? nullptr : profile->GetPrefs();
  auto report_uploader = std::make_unique<WebcompatReportUploader>(
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess());

  return std::make_unique<WebcompatReporterService>(
      prefs,
      std::make_unique<WebcompatReporterServiceDelegateImpl>(
          g_browser_process->component_updater(),
          g_brave_browser_process->ad_block_service(),
          HostContentSettingsMapFactory::GetForProfile(context),
          CookieSettingsFactory::GetForProfile(
          Profile::FromBrowserContext(context))),
      std::move(report_uploader));
}

}  // namespace webcompat_reporter
