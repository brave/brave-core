// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ntp_background_images/ntp_custom_background_images_service_factory.h"

#include <memory>

#include "brave/browser/ntp_background_images/ntp_custom_background_images_service_delegate.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/ntp_background_images/browser/ntp_custom_background_images_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace ntp_background_images {

// static
NTPCustomBackgroundImagesService*
NTPCustomBackgroundImagesServiceFactory::GetForContext(
    content::BrowserContext* context) {
  return static_cast<NTPCustomBackgroundImagesService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
NTPCustomBackgroundImagesServiceFactory*
NTPCustomBackgroundImagesServiceFactory::GetInstance() {
  return base::Singleton<NTPCustomBackgroundImagesServiceFactory>::get();
}

NTPCustomBackgroundImagesServiceFactory::
    NTPCustomBackgroundImagesServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "NTPCustomBackgroundImagesService",
          BrowserContextDependencyManager::GetInstance()) {}

NTPCustomBackgroundImagesServiceFactory::
    ~NTPCustomBackgroundImagesServiceFactory() = default;

KeyedService* NTPCustomBackgroundImagesServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  // Custom NTP background is only used in normal profile.
  if (!brave::IsRegularProfile(context))
    return nullptr;

  return new NTPCustomBackgroundImagesService(
      std::make_unique<NTPCustomBackgroundImagesServiceDelegate>(
          Profile::FromBrowserContext(context)));
}

}  // namespace ntp_background_images
