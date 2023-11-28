/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browser_context_keyed_service_factories.h"

#include "base/feature_list.h"
#include "brave/browser/brave_adaptive_captcha/brave_adaptive_captcha_service_factory.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_federated/brave_federated_service_factory.h"
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/brave_shields/ad_block_pref_service_factory.h"
#include "brave/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/browser/brave_wallet/bitcoin_wallet_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_ipfs_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/browser/brave_wallet/notifications/wallet_notification_service_factory.h"
#include "brave/browser/brave_wallet/simulation_service_factory.h"
#include "brave/browser/brave_wallet/swap_service_factory.h"
#include "brave/browser/brave_wallet/tx_service_factory.h"
#include "brave/browser/brave_wallet/zcash_wallet_service_factory.h"
#include "brave/browser/debounce/debounce_service_factory.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/browser/misc_metrics/extension_metrics_service_factory.h"
#include "brave/browser/misc_metrics/page_metrics_service_factory.h"
#include "brave/browser/ntp_background/view_counter_service_factory.h"
#include "brave/browser/permissions/permission_lifetime_manager_factory.h"
#include "brave/browser/profiles/brave_renderer_updater_factory.h"
#include "brave/browser/search_engines/search_engine_provider_service_factory.h"
#include "brave/browser/search_engines/search_engine_tracker.h"
#include "brave/browser/sync/brave_sync_alerts_service_factory.h"
#include "brave/browser/url_sanitizer/url_sanitizer_service_factory.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_perf_predictor/browser/named_third_party_registry_factory.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/commander/common/buildflags/buildflags.h"
#include "brave/components/greaselion/browser/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/request_otr/common/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN) || BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/skus/skus_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_GREASELION)
#include "brave/browser/greaselion/greaselion_service_factory.h"
#endif

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/infobars/brave_global_infobar_service_factory.h"
#include "brave/browser/ui/bookmark/bookmark_prefs_service_factory.h"
#include "brave/browser/ui/commands/accelerator_service_factory.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#include "brave/components/commands/common/features.h"
#else
#include "brave/browser/brave_shields/cookie_list_opt_in_service_factory.h"
#include "brave/browser/brave_shields/filter_list_service_factory.h"
#include "brave/browser/misc_metrics/misc_android_metrics_factory.h"
#include "brave/browser/ntp_background/android/ntp_background_images_bridge.h"
#endif

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/browser/ipfs/ipfs_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
#include "brave/browser/brave_wallet/brave_wallet_auto_pin_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_pin_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/playlist/common/features.h"
#endif

#if BUILDFLAG(ENABLE_COMMANDER)
#include "brave/browser/ui/commander/commander_service_factory.h"
#include "brave/components/commander/common/features.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/browser/speedreader/speedreader_service_factory.h"
#endif

#if BUILDFLAG(ENABLE_REQUEST_OTR)
#include "brave/browser/request_otr/request_otr_service_factory.h"
#endif

namespace brave {

void EnsureBrowserContextKeyedServiceFactoriesBuilt() {
  brave_adaptive_captcha::BraveAdaptiveCaptchaServiceFactory::GetInstance();
  brave_ads::AdsServiceFactory::GetInstance();
  brave_federated::BraveFederatedServiceFactory::GetInstance();
  brave_perf_predictor::NamedThirdPartyRegistryFactory::GetInstance();
  brave_rewards::RewardsServiceFactory::GetInstance();
  brave_shields::AdBlockPrefServiceFactory::GetInstance();
  debounce::DebounceServiceFactory::GetInstance();
  brave::URLSanitizerServiceFactory::GetInstance();
  BraveRendererUpdaterFactory::GetInstance();
  SearchEngineProviderServiceFactory::GetInstance();
  misc_metrics::PageMetricsServiceFactory::GetInstance();
#if BUILDFLAG(ENABLE_GREASELION)
  greaselion::GreaselionServiceFactory::GetInstance();
#endif
#if BUILDFLAG(ENABLE_TOR)
  TorProfileServiceFactory::GetInstance();
#endif
  SearchEngineTrackerFactory::GetInstance();
  ntp_background_images::ViewCounterServiceFactory::GetInstance();

#if !BUILDFLAG(IS_ANDROID)
  BookmarkPrefsServiceFactory::GetInstance();
#else
  brave_shields::CookieListOptInServiceFactory::GetInstance();
  brave_shields::FilterListServiceFactory::GetInstance();
  ntp_background_images::NTPBackgroundImagesBridgeFactory::GetInstance();
#endif

  brave_news::BraveNewsControllerFactory::GetInstance();
  brave_wallet::AssetRatioServiceFactory::GetInstance();
  brave_wallet::KeyringServiceFactory::GetInstance();
  brave_wallet::JsonRpcServiceFactory::GetInstance();
  brave_wallet::SwapServiceFactory::GetInstance();
  brave_wallet::SimulationServiceFactory::GetInstance();
#if !BUILDFLAG(IS_ANDROID)
  brave_wallet::WalletNotificationServiceFactory::GetInstance();
#endif
  brave_wallet::TxServiceFactory::GetInstance();
  brave_wallet::BraveWalletServiceFactory::GetInstance();

  if (brave_wallet::IsBitcoinEnabled()) {
    brave_wallet::BitcoinWalletServiceFactory::GetInstance();
  }
  if (brave_wallet::IsZCashEnabled()) {
    brave_wallet::ZCashWalletServiceFactory::GetInstance();
  }

#if !BUILDFLAG(IS_ANDROID)
  if (base::FeatureList::IsEnabled(commands::features::kBraveCommands)) {
    commands::AcceleratorServiceFactory::GetInstance();
  }
#endif

#if BUILDFLAG(ENABLE_COMMANDER)
  if (base::FeatureList::IsEnabled(features::kBraveCommander)) {
    commander::CommanderServiceFactory::GetInstance();
  }
#endif

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
  EthereumRemoteClientServiceFactory::GetInstance();
#endif

#if BUILDFLAG(ENABLE_IPFS)
  ipfs::IpfsServiceFactory::GetInstance();
#endif
  brave_wallet::BraveWalletIpfsServiceFactory::GetInstance();

#if BUILDFLAG(ENABLE_IPFS_LOCAL_NODE)
  brave_wallet::BraveWalletAutoPinServiceFactory::GetInstance();
  brave_wallet::BraveWalletPinServiceFactory::GetInstance();
#endif

  EphemeralStorageServiceFactory::GetInstance();
  PermissionLifetimeManagerFactory::GetInstance();
#if BUILDFLAG(ENABLE_BRAVE_VPN) || BUILDFLAG(ENABLE_AI_CHAT)
  skus::SkusServiceFactory::GetInstance();
#endif
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  brave_vpn::BraveVpnServiceFactory::GetInstance();
#endif
#if BUILDFLAG(ENABLE_PLAYLIST)
  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    playlist::PlaylistServiceFactory::GetInstance();
  }
#endif
#if BUILDFLAG(ENABLE_REQUEST_OTR)
  request_otr::RequestOTRServiceFactory::GetInstance();
#endif

  BraveSyncAlertsServiceFactory::GetInstance();

#if !BUILDFLAG(IS_ANDROID)
  if (base::FeatureList::IsEnabled(tabs::features::kBraveSharedPinnedTabs)) {
    SharedPinnedTabServiceFactory::GetInstance();
  }
  misc_metrics::ExtensionMetricsServiceFactory::GetInstance();
#endif

#if defined(TOOLKIT_VIEWS)
  sidebar::SidebarServiceFactory::GetInstance();
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
  speedreader::SpeedreaderServiceFactory::GetInstance();
#endif

#if BUILDFLAG(IS_ANDROID)
  misc_metrics::MiscAndroidMetricsFactory::GetInstance();
#endif

#if !BUILDFLAG(IS_ANDROID)
  BraveGlobalInfobarServiceFactory::GetInstance();
#endif
}

}  // namespace brave
