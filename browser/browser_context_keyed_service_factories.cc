/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browser_context_keyed_service_factories.h"

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_shields/ad_block_pref_service_factory.h"
#include "brave/browser/brave_shields/cookie_pref_service_factory.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/browser/ntp_background_images/view_counter_service_factory.h"
#include "brave/browser/permissions/permission_lifetime_manager_factory.h"
#include "brave/browser/search_engines/search_engine_provider_service_factory.h"
#include "brave/browser/search_engines/search_engine_tracker.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/greaselion/browser/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_GREASELION)
#include "brave/browser/greaselion/greaselion_service_factory.h"
#endif

#if !defined(OS_ANDROID)
#include "brave/browser/ui/bookmark/bookmark_prefs_service_factory.h"
#else
#include "brave/browser/ntp_background_images/android/ntp_background_images_bridge.h"
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#endif

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#endif

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service_factory.h"
#endif

#if BUILDFLAG(IPFS_ENABLED)
#include "brave/browser/ipfs/ipfs_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_service_factory.h"
#endif

namespace brave {

void EnsureBrowserContextKeyedServiceFactoriesBuilt() {
  brave_ads::AdsServiceFactory::GetInstance();
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  brave_rewards::RewardsServiceFactory::GetInstance();
#endif
  brave_shields::AdBlockPrefServiceFactory::GetInstance();
  brave_shields::CookiePrefServiceFactory::GetInstance();
#if BUILDFLAG(ENABLE_GREASELION)
  greaselion::GreaselionServiceFactory::GetInstance();
#endif
#if BUILDFLAG(ENABLE_TOR)
  TorProfileServiceFactory::GetInstance();
#endif
  SearchEngineProviderServiceFactory::GetInstance();
  SearchEngineTrackerFactory::GetInstance();
  ntp_background_images::ViewCounterServiceFactory::GetInstance();

#if !defined(OS_ANDROID)
  BookmarkPrefsServiceFactory::GetInstance();
#else
  ntp_background_images::NTPBackgroundImagesBridgeFactory::GetInstance();
#endif

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  brave_wallet::BraveWalletServiceFactory::GetInstance();
#endif

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
  EthereumRemoteClientServiceFactory::GetInstance();
#endif

#if BUILDFLAG(IPFS_ENABLED)
  ipfs::IpfsServiceFactory::GetInstance();
#endif
  PermissionLifetimeManagerFactory::GetInstance();
}

}  // namespace brave
