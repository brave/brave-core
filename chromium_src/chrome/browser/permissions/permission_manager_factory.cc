/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/permissions/permission_manager_factory.h"

#include "brave/browser/geolocation/brave_geolocation_permission_context_delegate.h"
#include "brave/browser/permissions/permission_lifetime_manager_factory.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/permissions/brave_permission_manager.h"
#include "brave/components/permissions/permission_lifetime_manager.h"
#include "components/permissions/features.h"

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
#include "brave/components/permissions/contexts/brave_ethereum_permission_context.h"
#endif

#define GeolocationPermissionContextDelegate \
  BraveGeolocationPermissionContextDelegate

#define BuildServiceInstanceFor BuildServiceInstanceFor_ChromiumImpl

#include "../../../../../chrome/browser/permissions/permission_manager_factory.cc"

#undef GeolocationPermissionContextDelegate
#undef BuildServiceInstanceFor

KeyedService* PermissionManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  auto permission_contexts = CreatePermissionContexts(profile);

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  permission_contexts[ContentSettingsType::BRAVE_ETHEREUM] =
      std::make_unique<permissions::BraveEthereumPermissionContext>(profile);
#endif

  if (base::FeatureList::IsEnabled(
          permissions::features::kPermissionLifetime)) {
    auto factory =
        base::BindRepeating(&PermissionLifetimeManagerFactory::GetForProfile);
    for (auto& permission_context : permission_contexts) {
      permission_context.second->SetPermissionLifetimeManagerFactory(factory);
    }
  }

  return new permissions::BravePermissionManager(
      profile, std::move(permission_contexts));
}
