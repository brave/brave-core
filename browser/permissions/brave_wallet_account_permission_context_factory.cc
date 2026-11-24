/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/permissions/brave_wallet_account_permission_context_factory.h"

#include <memory>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/permissions/contexts/brave_wallet_account_permission_context.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"

// static
permissions::BraveWalletAccountPermissionContext*
BraveWalletAccountPermissionContextFactory::GetForProfile(
    content::BrowserContext* profile) {
  return static_cast<permissions::BraveWalletAccountPermissionContext*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
BraveWalletAccountPermissionContextFactory*
BraveWalletAccountPermissionContextFactory::GetInstance() {
  static base::NoDestructor<BraveWalletAccountPermissionContextFactory>
      instance;
  return instance.get();
}

BraveWalletAccountPermissionContextFactory::
    BraveWalletAccountPermissionContextFactory()
    : ProfileKeyedServiceFactory(
          "BraveWalletAccountPermissionContextFactory",
          // Incognito grants live only in the off-the-record settings map, so
          // the off-the-record profile needs its own instance.
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              .Build()) {
  DependsOn(HostContentSettingsMapFactory::GetInstance());
  DependsOn(brave_wallet::BraveWalletServiceFactory::GetInstance());
}

BraveWalletAccountPermissionContextFactory::
    ~BraveWalletAccountPermissionContextFactory() = default;

std::unique_ptr<KeyedService> BraveWalletAccountPermissionContextFactory::
    BuildServiceInstanceForBrowserContext(
        content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(
          brave_wallet::features::kBraveWalletAccountObjectPermissions)) {
    return nullptr;
  }

  auto* profile = Profile::FromBrowserContext(context);

  HostContentSettingsMap* host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile);
  if (!host_content_settings_map) {
    return nullptr;
  }

  auto account_context =
      std::make_unique<permissions::BraveWalletAccountPermissionContext>(
          host_content_settings_map,
          brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
              context));
  // TODO(https://github.com/brave/brave-browser/issues/61873): Run the legacy
  // migration here, before the store is returned.
  return account_context;
}
