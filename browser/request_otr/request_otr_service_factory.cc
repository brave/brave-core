/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/request_otr/request_otr_service_factory.h"

#include <memory>
#include <string>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/request_otr/browser/request_otr_service.h"
#include "brave/components/request_otr/common/features.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"

namespace request_otr {

// static
RequestOTRServiceFactory* RequestOTRServiceFactory::GetInstance() {
  static base::NoDestructor<RequestOTRServiceFactory> instance;
  return instance.get();
}

RequestOTRService* RequestOTRServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<RequestOTRService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

RequestOTRServiceFactory::RequestOTRServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "RequestOTRService",
          BrowserContextDependencyManager::GetInstance()) {}

RequestOTRServiceFactory::~RequestOTRServiceFactory() = default;

std::unique_ptr<KeyedService>
RequestOTRServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  // Don't create service is request_otr feature is disabled
  if (!base::FeatureList::IsEnabled(
          request_otr::features::kBraveRequestOTRTab)) {
    return nullptr;
  }

  auto service = std::make_unique<RequestOTRService>(
      Profile::FromBrowserContext(context)->GetPrefs());
  request_otr::RequestOTRComponentInstallerPolicy* component_installer =
      nullptr;
  // Brave browser process may be null if we are being created within a unit
  // test.
  if (g_brave_browser_process) {
    component_installer =
        g_brave_browser_process->request_otr_component_installer();
  }
  if (component_installer) {
    component_installer->AddObserver(service.get());
  }
  return service;
}

content::BrowserContext* RequestOTRServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool RequestOTRServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

}  // namespace request_otr
