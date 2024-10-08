/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/speedreader/speedreader_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace speedreader {

// static
SpeedreaderServiceFactory* SpeedreaderServiceFactory::GetInstance() {
  static base::NoDestructor<SpeedreaderServiceFactory> instance;
  return instance.get();
}

SpeedreaderService* SpeedreaderServiceFactory::GetForBrowserContext(
    content::BrowserContext* browser_context) {
  return static_cast<SpeedreaderService*>(
      SpeedreaderServiceFactory::GetInstance()->GetServiceForBrowserContext(
          browser_context, true /*create*/));
}

SpeedreaderServiceFactory::SpeedreaderServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "SpeedreaderService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(HostContentSettingsMapFactory::GetInstance());
}

SpeedreaderServiceFactory::~SpeedreaderServiceFactory() = default;

content::BrowserContext* SpeedreaderServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}

std::unique_ptr<KeyedService>
SpeedreaderServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  if (!features::IsSpeedreaderEnabled()) {
    return {};
  }
  return std::make_unique<SpeedreaderService>(
      context, HostContentSettingsMapFactory::GetForProfile(context));
}

bool SpeedreaderServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace speedreader
