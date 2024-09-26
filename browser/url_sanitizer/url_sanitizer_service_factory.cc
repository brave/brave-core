/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/url_sanitizer/url_sanitizer_service_factory.h"

#include <memory>
#include <string>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_component_installer.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"

namespace brave {

// static
URLSanitizerServiceFactory* URLSanitizerServiceFactory::GetInstance() {
  static base::NoDestructor<URLSanitizerServiceFactory> instance;
  return instance.get();
}

URLSanitizerService* URLSanitizerServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<URLSanitizerService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

#if BUILDFLAG(IS_ANDROID)
// static
mojo::PendingRemote<url_sanitizer::mojom::UrlSanitizerService>
URLSanitizerServiceFactory::GetForContext(content::BrowserContext* context) {
  return static_cast<URLSanitizerService*>(
             GetInstance()->GetServiceForBrowserContext(context, true))
      ->MakeRemote();
}
#endif  // # BUILDFLAG(IS_ANDROID)

URLSanitizerServiceFactory::URLSanitizerServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "URLSanitizerService",
          BrowserContextDependencyManager::GetInstance()) {}

URLSanitizerServiceFactory::~URLSanitizerServiceFactory() = default;

std::unique_ptr<KeyedService>
URLSanitizerServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto service = std::make_unique<URLSanitizerService>();
  if (g_brave_browser_process &&
      g_brave_browser_process->URLSanitizerComponentInstaller()) {
    g_brave_browser_process->URLSanitizerComponentInstaller()->AddObserver(
        service.get());
  }
  return service;
}

bool URLSanitizerServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

content::BrowserContext* URLSanitizerServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return GetBrowserContextRedirectedInIncognito(context);
}

}  // namespace brave
