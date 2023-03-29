/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_web_ui_controller_factory.h"

#include <memory>

#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/browser/ui/webui/brave_adblock_internals_ui.h"
#include "brave/browser/ui/webui/brave_adblock_ui.h"
#include "brave/browser/ui/webui/brave_rewards_internals_ui.h"
#include "brave/browser/ui/webui/brave_rewards_page_ui.h"
#include "brave/browser/ui/webui/brave_tip_ui.h"
#include "brave/browser/ui/webui/webcompat_reporter_ui.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_rewards/common/rewards_util.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/ui/webui/brave_rewards/rewards_panel_ui.h"
#include "brave/browser/ui/webui/brave_settings_ui.h"
#include "brave/browser/ui/webui/brave_shields/cookie_list_opt_in_ui.h"
#include "brave/browser/ui/webui/brave_shields/shields_panel_ui.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_page_ui.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_panel_ui.h"
#include "brave/browser/ui/webui/commands_ui.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_ui.h"
#include "brave/browser/ui/webui/private_new_tab_page/brave_private_new_tab_ui.h"
#include "brave/browser/ui/webui/speedreader/speedreader_panel_ui.h"
#include "brave/browser/ui/webui/welcome_page/brave_welcome_ui.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_util.h"
#include "brave/components/commands/common/features.h"
#endif

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/webui/brave_wallet/android/swap_page_ui.h"
#endif

#include "brave/browser/brave_vpn/vpn_utils.h"
#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ui/webui/ethereum_remote_client/ethereum_remote_client_ui.h"
#endif

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/browser/ui/webui/ipfs_ui.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_utils.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
#include "brave/browser/ui/webui/playlist_ui.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/ui/webui/tor_internals_ui.h"
#endif

using content::WebUI;
using content::WebUIController;

namespace {

// A function for creating a new WebUI. The caller owns the return value, which
// may be NULL (for example, if the URL refers to an non-existent extension).
typedef WebUIController* (*WebUIFactoryFunction)(WebUI* web_ui,
                                                 const GURL& url);

WebUIController* NewWebUI(WebUI* web_ui, const GURL& url) {
  auto host = url.host_piece();
  Profile* profile = Profile::FromBrowserContext(
      web_ui->GetWebContents()->GetBrowserContext());
  if (host == kAdblockHost) {
    return new BraveAdblockUI(web_ui, url.host());
  } else if (host == kAdblockInternalsHost) {
    return new BraveAdblockInternalsUI(web_ui, url.host());
  } else if (host == kWebcompatReporterHost) {
    return new WebcompatReporterUI(web_ui, url.host());
#if BUILDFLAG(ENABLE_IPFS)
  } else if (host == kIPFSWebUIHost &&
             ipfs::IpfsServiceFactory::IsIpfsEnabled(profile)) {
    return new IPFSUI(web_ui, url.host());
#endif
#if !BUILDFLAG(IS_ANDROID)
  } else if (host == kCommandsHost &&
             base::FeatureList::IsEnabled(commands::features::kBraveCommands)) {
    return new commands::CommandsUI(web_ui, url.host());
  } else if (host == kWalletPageHost &&
             // We don't want to check for supported profile type here because
             // we want private windows to redirect to the regular profile.
             // Guest session will just show an error page.
             brave_wallet::IsAllowed(profile->GetPrefs())) {
    if (brave_wallet::IsNativeWalletEnabled()) {
      auto default_wallet =
          brave_wallet::GetDefaultEthereumWallet(profile->GetPrefs());
      if (default_wallet == brave_wallet::mojom::DefaultWallet::CryptoWallets)
        return new EthereumRemoteClientUI(web_ui, url.host());
      return new WalletPageUI(web_ui);
    }
#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
    return new EthereumRemoteClientUI(web_ui, url.host());
#endif
  } else if (host == kWalletPanelHost &&
             brave_wallet::IsAllowedForContext(profile)) {
    return new WalletPanelUI(web_ui);
#endif  // BUILDFLAG(OS_ANDROID)
  } else if (host == kRewardsPageHost &&
             // We don't want to check for supported profile type here because
             // we want private windows to redirect to the regular profile.
             // Additionally, if Rewards aren't supported because of the OFAC
             // sanctions we want to show the page with an appropriate error
             // message.
             // Guest session will just show an error page.
             brave_rewards::IsSupported(
                 profile->GetPrefs(),
                 brave_rewards::IsSupportedOptions::kSkipRegionCheck)) {
    return new BraveRewardsPageUI(web_ui, url.host());
  } else if (host == kRewardsInternalsHost &&
             brave_rewards::IsSupportedForProfile(profile)) {
    return new BraveRewardsInternalsUI(web_ui, url.host());
#if !BUILDFLAG(IS_ANDROID)
  } else if (host == kTipHost &&
             brave_rewards::IsSupportedForProfile(profile)) {
    return new BraveTipUI(web_ui, url.host());
  } else if (host == kBraveRewardsPanelHost &&
             brave_rewards::IsSupportedForProfile(profile)) {
    return new RewardsPanelUI(web_ui);
#endif  // !BUILDFLAG(IS_ANDROID)
#if !BUILDFLAG(IS_ANDROID)
  } else if (host == kWelcomeHost) {
    return new BraveWelcomeUI(web_ui, url.host());
  } else if (host == chrome::kChromeUISettingsHost) {
    return new BraveSettingsUI(web_ui, url.host());
  } else if (host == chrome::kChromeUINewTabHost) {
    if (profile->IsIncognitoProfile() || profile->IsTor() ||
        profile->IsGuestSession()) {
      return new BravePrivateNewTabUI(web_ui, url.host());
    }
    return new BraveNewTabUI(web_ui, url.host());
  } else if (host == kShieldsPanelHost) {
    return new ShieldsPanelUI(web_ui);
  } else if (host == kSpeedreaderPanelHost) {
    return new SpeedreaderPanelUI(web_ui, url.host());
  } else if (host == kCookieListOptInHost) {
    if (base::FeatureList::IsEnabled(
            brave_shields::features::kBraveAdblockCookieListOptIn)) {
      return new CookieListOptInUI(web_ui);
    }
#endif  // !BUILDFLAG(IS_ANDROID)
#if BUILDFLAG(ENABLE_TOR)
  } else if (host == kTorInternalsHost) {
    return new TorInternalsUI(web_ui, url.host());
#endif
#if BUILDFLAG(IS_ANDROID)
  } else if (url.is_valid() &&
             (url.spec() == base::StringPrintf("%s://%s",
                                               content::kChromeUIScheme,
                                               kWalletSwapPagePath) ||
              url.spec() == base::StringPrintf("%s://%s",
                                               content::kBraveUIScheme,
                                               kWalletSwapPagePath))) {
    return new SwapPageUI(web_ui, url.host());
#endif
  }
  return nullptr;
}

// Returns a function that can be used to create the right type of WebUI for a
// tab, based on its URL. Returns NULL if the URL doesn't have WebUI associated
// with it.
WebUIFactoryFunction GetWebUIFactoryFunction(WebUI* web_ui, const GURL& url) {
  if (url.host_piece() == kAdblockHost ||
      url.host_piece() == kAdblockInternalsHost ||
      url.host_piece() == kWebcompatReporterHost ||
#if BUILDFLAG(ENABLE_IPFS)
      (url.host_piece() == kIPFSWebUIHost &&
       base::FeatureList::IsEnabled(ipfs::features::kIpfsFeature)) ||
#endif  // BUILDFLAG(ENABLE_IPFS)
#if !BUILDFLAG(IS_ANDROID)
      url.host_piece() == kWalletPanelHost ||
      url.host_piece() == kWalletPageHost ||
#endif
#if BUILDFLAG(IS_ANDROID)
      (url.is_valid() &&
       (url.spec() == base::StringPrintf("%s://%s", content::kChromeUIScheme,
                                         kWalletSwapPagePath) ||
        url.spec() == base::StringPrintf("%s://%s", content::kBraveUIScheme,
                                         kWalletSwapPagePath))) ||
#endif  // BUILDFLAG(IS_ANDROID)
      url.host_piece() == kShieldsPanelHost ||
      (url.host_piece() == kCookieListOptInHost &&
       base::FeatureList::IsEnabled(
           brave_shields::features::kBraveAdblockCookieListOptIn)) ||
      url.host_piece() == kRewardsPageHost ||
      url.host_piece() == kRewardsInternalsHost ||
#if !BUILDFLAG(IS_ANDROID)
      url.host_piece() == kTipHost ||
      url.host_piece() == kBraveRewardsPanelHost ||
      url.host_piece() == kSpeedreaderPanelHost ||
      url.host_piece() == kCommandsHost ||
#endif
#if BUILDFLAG(ENABLE_TOR)
      url.host_piece() == kTorInternalsHost ||
#endif
      url.host_piece() == kWelcomeHost ||
      url.host_piece() == chrome::kChromeUIWelcomeURL ||
#if !BUILDFLAG(IS_ANDROID)
      // On Android New Tab is a native page implemented in Java, so no need in
      // WebUI.
      url.host_piece() == chrome::kChromeUINewTabHost ||
#endif  // !BUILDFLAG(IS_ANDROID)
      url.host_piece() == chrome::kChromeUISettingsHost) {
    return &NewWebUI;
  }

  return nullptr;
}

bool ShouldBlockRewardsWebUI(content::BrowserContext* browser_context,
                             const GURL& url) {
  if (url.host_piece() != kRewardsPageHost &&
#if !BUILDFLAG(IS_ANDROID)
      url.host_piece() != kTipHost &&
      url.host_piece() != kBraveRewardsPanelHost &&
#endif  // !BUILDFLAG(IS_ANDROID)
      url.host_piece() != kRewardsInternalsHost) {
    return false;
  }

  Profile* profile = Profile::FromBrowserContext(browser_context);
  if (profile) {
    if (!brave_rewards::IsSupportedForProfile(
            profile, url.host_piece() == kRewardsPageHost
                         ? brave_rewards::IsSupportedOptions::kSkipRegionCheck
                         : brave_rewards::IsSupportedOptions::kNone)) {
      return true;
    }
#if BUILDFLAG(IS_ANDROID)
    auto* prefs = profile->GetPrefs();
    if (prefs && prefs->GetBoolean(kSafetynetCheckFailed)) {
      return true;
    }
#endif  // BUILDFLAG(IS_ANDROID)
  }
  return false;
}

}  // namespace

WebUI::TypeID BraveWebUIControllerFactory::GetWebUIType(
    content::BrowserContext* browser_context,
    const GURL& url) {
  if (ShouldBlockRewardsWebUI(browser_context, url)) {
    return WebUI::kNoWebUI;
  }
#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  if (playlist::PlaylistUI::ShouldBlockPlaylistWebUI(browser_context, url))
    return WebUI::kNoWebUI;
#endif
  WebUIFactoryFunction function = GetWebUIFactoryFunction(nullptr, url);
  if (function) {
    return reinterpret_cast<WebUI::TypeID>(function);
  }
  return ChromeWebUIControllerFactory::GetWebUIType(browser_context, url);
}

std::unique_ptr<WebUIController>
BraveWebUIControllerFactory::CreateWebUIControllerForURL(WebUI* web_ui,
                                                         const GURL& url) {
  WebUIFactoryFunction function = GetWebUIFactoryFunction(web_ui, url);
  if (!function) {
    return ChromeWebUIControllerFactory::CreateWebUIControllerForURL(web_ui,
                                                                     url);
  }

  return base::WrapUnique((*function)(web_ui, url));
}

// static
BraveWebUIControllerFactory* BraveWebUIControllerFactory::GetInstance() {
  return base::Singleton<BraveWebUIControllerFactory>::get();
}

BraveWebUIControllerFactory::BraveWebUIControllerFactory() = default;

BraveWebUIControllerFactory::~BraveWebUIControllerFactory() = default;
