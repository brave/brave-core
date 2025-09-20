/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/psst/brave_psst_permission_context_factory.h"

#include "brave/browser/psst/brave_psst_permission_context.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"

namespace psst {

// static
BravePsstPermissionContextFactory*
BravePsstPermissionContextFactory::GetInstance() {
  static base::NoDestructor<BravePsstPermissionContextFactory> instance;
  return instance.get();
}

// static
BravePsstPermissionContext*
BravePsstPermissionContextFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  CHECK(context);
  return static_cast<BravePsstPermissionContext*>(
      GetInstance()->GetServiceForContext(context, true));
}

BravePsstPermissionContextFactory::BravePsstPermissionContextFactory()
    : ProfileKeyedServiceFactory("BravePsstPermissionService") {
  DependsOn(HostContentSettingsMapFactory::GetInstance());
}

BravePsstPermissionContextFactory::~BravePsstPermissionContextFactory() =
    default;

std::unique_ptr<KeyedService>
BravePsstPermissionContextFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  CHECK(context);
  auto* profile = Profile::FromBrowserContext(context);
  auto* host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile);

  CHECK(host_content_settings_map);

  return std::make_unique<BravePsstPermissionContext>(
      host_content_settings_map);
}

}  // namespace psst
