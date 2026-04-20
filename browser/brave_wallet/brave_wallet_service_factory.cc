/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"

#include <memory>
#include <optional>

#include "base/no_destructor.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/storage_partition.h"

namespace brave_wallet {

namespace {
std::optional<bool> g_null_in_tests_override;

bool CanBuildWalletServiceInstanceForBrowserContext(
    content::BrowserContext* context) {
  if (!context) {
    return false;
  }

  auto* profile = Profile::FromBrowserContext(context);

  return CanBuildWalletServiceInstance(
      user_prefs::UserPrefs::Get(context), profile->IsRegularProfile(),
      profile->IsIncognitoProfile(), profile->IsTor());
}

}  // namespace

// static
BraveWalletServiceFactory* BraveWalletServiceFactory::GetInstance() {
  static base::NoDestructor<BraveWalletServiceFactory> instance;
  return instance.get();
}

// static
BraveWalletService* BraveWalletServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  return static_cast<BraveWalletService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

BraveWalletServiceFactory::BraveWalletServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveWalletService",
          BrowserContextDependencyManager::GetInstance()) {}

BraveWalletServiceFactory::~BraveWalletServiceFactory() = default;

std::unique_ptr<KeyedService>
BraveWalletServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  if (!CanBuildWalletServiceInstanceForBrowserContext(context)) {
    return nullptr;
  }

  return std::make_unique<BraveWalletService>(
      context->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess(),
      BraveWalletServiceDelegate::Create(context),
      user_prefs::UserPrefs::Get(context), g_browser_process->local_state());
}

content::BrowserContext* BraveWalletServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return context;
}

bool BraveWalletServiceFactory::ServiceIsNULLWhileTesting() const {
  if (g_null_in_tests_override.has_value()) {
    return g_null_in_tests_override.value();
  }

  // Using BrowserTabStripTracker in BraveWalletServiceDelegate can cause
  // upstream tests to crash if they don't have the correct order of
  // creation/destruction of profiles and calls to
  // SetUpGlobalFeaturesForTesting/TearDownGlobalFeaturesForTesting because
  // BrowserTabStripTracker relies on global features. By default, make this
  // service NULL for testing and create it manually in tests when needed.
  return true;
}

BraveWalletServiceFactory::NotNullForTesting::NotNullForTesting() {
  g_null_in_tests_override = false;
}
BraveWalletServiceFactory::NotNullForTesting::~NotNullForTesting() {
  g_null_in_tests_override.reset();
}

}  // namespace brave_wallet
