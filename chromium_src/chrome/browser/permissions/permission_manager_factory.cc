/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/permissions/permission_manager_factory.h"

#include "brave/browser/geolocation/brave_geolocation_permission_context_delegate.h"
#include "brave/browser/permissions/permission_lifetime_manager_factory.h"
#include "brave/components/permissions/brave_permission_manager.h"
#include "brave/components/permissions/contexts/brave_google_sign_in_permission_context.h"
#include "brave/components/permissions/contexts/brave_localhost_permission_context.h"
#include "brave/components/permissions/contexts/brave_open_ai_chat_permission_context.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "brave/components/permissions/permission_lifetime_manager.h"
#include "components/permissions/features.h"

#define GeolocationPermissionContextDelegate \
  BraveGeolocationPermissionContextDelegate

#define BuildServiceInstanceForBrowserContext \
  BuildServiceInstanceForBrowserContext_ChromiumImpl

#include "src/chrome/browser/permissions/permission_manager_factory.cc"

#undef GeolocationPermissionContextDelegate
#undef BuildServiceInstanceForBrowserContext

std::unique_ptr<KeyedService>
PermissionManagerFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  auto permission_contexts = CreatePermissionContexts(profile);

  permission_contexts[ContentSettingsType::BRAVE_ETHEREUM] =
      std::make_unique<permissions::BraveWalletPermissionContext>(
          profile, ContentSettingsType::BRAVE_ETHEREUM);
  permission_contexts[ContentSettingsType::BRAVE_SOLANA] =
      std::make_unique<permissions::BraveWalletPermissionContext>(
          profile, ContentSettingsType::BRAVE_SOLANA);
  permission_contexts[ContentSettingsType::BRAVE_GOOGLE_SIGN_IN] =
      std::make_unique<permissions::BraveGoogleSignInPermissionContext>(
          profile);
  permission_contexts[ContentSettingsType::BRAVE_LOCALHOST_ACCESS] =
      std::make_unique<permissions::BraveLocalhostPermissionContext>(profile);
  permission_contexts[ContentSettingsType::BRAVE_OPEN_AI_CHAT] =
      std::make_unique<permissions::BraveOpenAIChatPermissionContext>(profile);

  if (base::FeatureList::IsEnabled(
          permissions::features::kPermissionLifetime)) {
    auto factory =
        base::BindRepeating(&PermissionLifetimeManagerFactory::GetForProfile);
    for (auto& permission_context : permission_contexts) {
      permission_context.second->SetPermissionLifetimeManagerFactory(factory);
    }
  }

  return std::make_unique<permissions::BravePermissionManager>(
      profile, std::move(permission_contexts));
}
