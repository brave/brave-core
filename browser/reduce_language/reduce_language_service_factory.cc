/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/reduce_language/reduce_language_service_factory.h"

#include <memory>
#include <string>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/reduce_language/browser/reduce_language_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"

namespace reduce_language {

// static
ReduceLanguageServiceFactory* ReduceLanguageServiceFactory::GetInstance() {
  static base::NoDestructor<ReduceLanguageServiceFactory> instance;
  return instance.get();
}

ReduceLanguageService* ReduceLanguageServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<ReduceLanguageService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

ReduceLanguageServiceFactory::ReduceLanguageServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "ReduceLanguageService",
          BrowserContextDependencyManager::GetInstance()) {}

ReduceLanguageServiceFactory::~ReduceLanguageServiceFactory() = default;

KeyedService* ReduceLanguageServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  // Don't create service is reduce_language feature is disabled
  if (!base::FeatureList::IsEnabled(
          brave_shields::features::kBraveReduceLanguage)) {
    return nullptr;
  }

  ReduceLanguageService* service = new ReduceLanguageService();
  reduce_language::ReduceLanguageComponentInstallerPolicy* component_installer =
      nullptr;
  // Brave browser process may be null if we are being created within a unit
  // test.
  if (g_brave_browser_process) {
    component_installer =
        g_brave_browser_process->reduce_language_component_installer();
  }
  if (component_installer) {
    component_installer->AddObserver(service);
  }
  return service;
}

content::BrowserContext* ReduceLanguageServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool ReduceLanguageServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

}  // namespace reduce_language
