/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ntp_sponsored_images/ntp_sponsored_images_service_factory.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/ntp_sponsored_images/ntp_sponsored_images_service.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

// static
NTPSponsoredImagesServiceFactory*
NTPSponsoredImagesServiceFactory::GetInstance() {
  return base::Singleton<NTPSponsoredImagesServiceFactory>::get();
}

NTPSponsoredImagesServiceFactory::NTPSponsoredImagesServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "NTPSponsoredImagesService",
          BrowserContextDependencyManager::GetInstance()) {
}

NTPSponsoredImagesServiceFactory::~NTPSponsoredImagesServiceFactory() = default;

KeyedService* NTPSponsoredImagesServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new NTPSponsoredImagesService(
      context,
      g_brave_browser_process->ntp_sponsored_images_component_manager());
}

content::BrowserContext*
NTPSponsoredImagesServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool NTPSponsoredImagesServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

bool
NTPSponsoredImagesServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  // Service should be initialized when profile is created to set proper
  // provider to TemplateURLService.
  return true;
}
