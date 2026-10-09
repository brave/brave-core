/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/permissions/brave_wallet_account_chooser_contexts_factory.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/browser/permissions/brave_wallet_account_chooser_contexts.h"
#include "brave/browser/permissions/permission_origin_lifetime_monitor_impl.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/permissions/contexts/brave_wallet_account_chooser_context.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/permissions/features.h"

namespace brave_wallet {

namespace {

constexpr mojom::CoinType kPermissionedCoins[] = {
    mojom::CoinType::ETH, mojom::CoinType::SOL, mojom::CoinType::ADA};

}  // namespace

// static
BraveWalletAccountChooserContexts*
BraveWalletAccountChooserContextsFactory::GetForContext(
    content::BrowserContext* context) {
  return static_cast<BraveWalletAccountChooserContexts*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
BraveWalletAccountChooserContextsFactory*
BraveWalletAccountChooserContextsFactory::GetInstance() {
  static base::NoDestructor<BraveWalletAccountChooserContextsFactory> instance;
  return instance.get();
}

BraveWalletAccountChooserContextsFactory::
    BraveWalletAccountChooserContextsFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveWalletAccountChooserContextsFactory",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(EphemeralStorageServiceFactory::GetInstance());
  DependsOn(HostContentSettingsMapFactory::GetInstance());
}

BraveWalletAccountChooserContextsFactory::
    ~BraveWalletAccountChooserContextsFactory() = default;

std::unique_ptr<KeyedService>
BraveWalletAccountChooserContextsFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto* profile = Profile::FromBrowserContext(context);
  // Null for irregular profiles such as the System Profile.
  auto* host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(profile);
  if (!host_content_settings_map) {
    return nullptr;
  }

  // Without the lifetime feature there is no "until page close" option in the
  // prompt, so no monitor is needed. Passing null also makes the context
  // refuse a zero lifetime rather than quietly granting it permanently.
  const bool lifetime_enabled =
      base::FeatureList::IsEnabled(permissions::features::kPermissionLifetime);

  BraveWalletAccountChooserContexts::ContextMap contexts;
  for (const auto coin : kPermissionedCoins) {
    std::unique_ptr<permissions::PermissionOriginLifetimeMonitor> monitor;
    if (lifetime_enabled) {
      monitor =
          std::make_unique<permissions::PermissionOriginLifetimeMonitorImpl>(
              context);
    }
    contexts[coin] =
        std::make_unique<permissions::BraveWalletAccountChooserContext>(
            coin, host_content_settings_map, std::move(monitor));
  }

  return std::make_unique<BraveWalletAccountChooserContexts>(
      std::move(contexts));
}

content::BrowserContext*
BraveWalletAccountChooserContextsFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // Private windows get their own instance so a regular-profile connection is
  // never visible there; the data content settings types are also registered
  // DONT_INHERIT_IN_INCOGNITO.
  return ::GetBrowserContextOwnInstanceInIncognito(context);
}

}  // namespace brave_wallet
