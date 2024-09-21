/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/webcompat_reporter/webcompat_reporter_service_factory.h"

#include <utility>

#include "base/no_destructor.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace webcompat_reporter {

// static
WebcompatReporterServiceFactory* WebcompatReporterServiceFactory::GetInstance() {
  static base::NoDestructor<WebcompatReporterServiceFactory> instance;
  return instance.get();
}

// static
mojo::PendingRemote<mojom::WebcompatReporterHandler>
WebcompatReporterServiceFactory::GetForContext(content::BrowserContext* context) {
  return static_cast<WebcompatReporterService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}

// // static
// WebcompatReporterService* WebcompatReporterServiceFactory::GetServiceForContext(
//     content::BrowserContext* context) {
//   return static_cast<WebcompatReporterService*>(
//       GetInstance()->GetServiceForBrowserContext(context, true));
// }

// // static
// void WebcompatReporterServiceFactory::BindForContext(
//     content::BrowserContext* context,
//     mojo::PendingReceiver<mojom::FilterListAndroidHandler> receiver) {
//   auto* filter_list_opt_in_service =
//       WebcompatReporterServiceFactory::GetServiceForContext(context);
//   if (filter_list_opt_in_service) {
//     filter_list_opt_in_service->Bind(std::move(receiver));
//   }
// }

WebcompatReporterServiceFactory::WebcompatReporterServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "WebcompatReporterService",
          BrowserContextDependencyManager::GetInstance()) {}

WebcompatReporterServiceFactory::~WebcompatReporterServiceFactory() = default;

KeyedService* WebcompatReporterServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return g_brave_browser_process->webcompat_reporter_service();
}

content::BrowserContext* WebcompatReporterServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace webcompat_reporter
