/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_web_ui_controller_factory.h"

#include <memory>
#include <string>

#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/browser/ui/webui/brave_rewards/rewards_page_ui.h"
#include "brave/browser/ui/webui/brave_rewards/rewards_web_ui_utils.h"
#include "brave/browser/ui/webui/brave_rewards_internals_ui.h"
#include "brave/browser/ui/webui/brave_rewards_page_ui.h"
#include "brave/browser/ui/webui/skus_internals_ui.h"
#include "brave/components/ai_rewriter/common/buildflags/buildflags.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/skus/common/features.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "components/optimization_guide/core/optimization_guide_features.h"
#include "components/optimization_guide/optimization_guide_internals/webui/url_constants.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_utils.h"
#include "url/gurl.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/ui/webui/brave_news_internals/brave_news_internals_ui.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_page_ui.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_ui.h"
#include "brave/browser/ui/webui/welcome_page/brave_welcome_ui.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/commands/common/features.h"
#endif

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/ui/webui/brave_wallet/android/android_wallet_page_ui.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#endif

#include "brave/browser/brave_vpn/vpn_utils.h"
#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ui/webui/ethereum_remote_client/ethereum_remote_client_ui.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
#include "brave/browser/ui/webui/playlist_ui.h"
#include "brave/components/playlist/common/features.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/ui/webui/tor_internals_ui.h"
#endif

#if BUILDFLAG(ENABLE_AI_REWRITER)
#include "brave/browser/ui/webui/ai_rewriter/ai_rewriter_ui.h"
#include "brave/components/ai_rewriter/common/features.h"
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
  if (host == kSkusInternalsHost) {
    return new SkusInternalsUI(web_ui, url.host());
#if !BUILDFLAG(IS_ANDROID)
  } else if (host == kWalletPageHost &&
             brave_wallet::IsAllowedForContext(profile)) {
    if (brave_wallet::IsNativeWalletEnabled()) {
      auto default_wallet =
          brave_wallet::GetDefaultEthereumWallet(profile->GetPrefs());
      if (default_wallet == brave_wallet::mojom::DefaultWallet::CryptoWallets) {
        return new EthereumRemoteClientUI(web_ui, url.host());
      }
      return new WalletPageUI(web_ui);
    }
#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
    return new EthereumRemoteClientUI(web_ui, url.host());
#endif
#endif  // !BUILDFLAG(OS_ANDROID)
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
    if (base::FeatureList::IsEnabled(
            brave_rewards::features::kNewRewardsUIFeature)) {
      return new brave_rewards::RewardsPageUI(web_ui, url.host());
    }
    return new BraveRewardsPageUI(web_ui, url.host());
  } else if (host == kRewardsInternalsHost &&
             brave_rewards::IsSupportedForProfile(profile)) {
    return new BraveRewardsInternalsUI(web_ui, url.host());
#if !BUILDFLAG(IS_ANDROID)
  } else if (base::FeatureList::IsEnabled(
                 brave_news::features::kBraveNewsFeedUpdate) &&
             host == kBraveNewsInternalsHost) {
    return new BraveNewsInternalsUI(
        web_ui, url.host(),
        brave_news::BraveNewsControllerFactory::GetForBrowserContext(profile));
  } else if (host == kWelcomeHost && !profile->IsGuestSession()) {
    return new BraveWelcomeUI(web_ui, url.host());
  } else if (host == chrome::kChromeUINewTabHost) {
    // For private profiles the webui handling kChromeUINewTabHost is configured
    // with RegisterChromeWebUIConfigs, so we should not get called here with a
    // private profile.
    DCHECK(!profile->IsIncognitoProfile() && !profile->IsTor() &&
           !profile->IsGuestSession());
    // We will need to follow up on transitioning BraveNewTabUI to using
    // WebUIConfig. Currently, we can't add both BravePrivateNewTabUI and
    // BraveNewTabUI configs in RegisterChromeWebUIConfigs because they use the
    // same origin (content::kChromeUIScheme + chrome::kChromeUINewTabHost).
    return new BraveNewTabUI(web_ui, url.host());
#endif  // !BUILDFLAG(IS_ANDROID)
#if BUILDFLAG(ENABLE_TOR)
  } else if (host == kTorInternalsHost) {
    return new TorInternalsUI(web_ui, url.host());
#endif
#if BUILDFLAG(IS_ANDROID)
  } else if (url.is_valid() && url.host() == kWalletPageHost) {
    return new AndroidWalletPageUI(web_ui, url);
#endif
#if BUILDFLAG(ENABLE_AI_REWRITER)
  } else if (host == kRewriterUIHost) {
    if (ai_rewriter::features::IsAIRewriterEnabled()) {
      return new ai_rewriter::AIRewriterUI(web_ui);
    }
#endif
  }
  return nullptr;
}

// Returns a function that can be used to create the right type of WebUI for a
// tab, based on its URL. Returns NULL if the URL doesn't have WebUI associated
// with it.
WebUIFactoryFunction GetWebUIFactoryFunction(WebUI* web_ui,
                                             Profile* profile,
                                             const GURL& url) {
  // This will get called a lot to check all URLs, so do a quick check of other
  // schemes to filter out most URLs.
  //
  // This has a narrow scoper scope than content::HasWebUIScheme(url) which also
  // allows both `chrome-untrusted` and `chrome-devtools`.
  if (!url.SchemeIs(content::kBraveUIScheme) &&
      !url.SchemeIs(content::kChromeUIScheme)) {
    return nullptr;
  }

  if ((url.host_piece() == kSkusInternalsHost &&
       base::FeatureList::IsEnabled(skus::features::kSkusFeature)) ||
#if BUILDFLAG(IS_ANDROID)
      (url.is_valid() && url.host_piece() == kWalletPageHost) ||
#else
      (base::FeatureList::IsEnabled(
           brave_news::features::kBraveNewsFeedUpdate) &&
       url.host_piece() == kBraveNewsInternalsHost) ||
      (url.host_piece() == kWalletPageHost &&
       brave_wallet::IsAllowedForContext(profile)) ||
      // On Android New Tab is a native page implemented in Java, so no need
      // in WebUI.
      url.host_piece() == chrome::kChromeUINewTabHost ||
      url.host_piece() == chrome::kChromeUISettingsHost ||
      ((url.host_piece() == kWelcomeHost ||
        url.host_piece() == chrome::kChromeUIWelcomeURL) &&
       !profile->IsGuestSession()) ||
#endif  // BUILDFLAG(IS_ANDROID)
#if BUILDFLAG(ENABLE_TOR)
      url.host_piece() == kTorInternalsHost ||
#endif
#if BUILDFLAG(ENABLE_AI_REWRITER)
      (url.host_piece() == kRewriterUIHost &&
       ai_rewriter::features::IsAIRewriterEnabled()) ||
#endif
      url.host_piece() == kRewardsPageHost ||
      url.host_piece() == kRewardsInternalsHost) {
    return &NewWebUI;
  }

  return nullptr;
}

#if BUILDFLAG(IS_ANDROID)
bool ShouldBlockWalletWebUI(content::BrowserContext* browser_context,
                            const GURL& url) {
  if (!url.is_valid() || url.host() != kWalletPageHost) {
    return false;
  }
  Profile* profile = Profile::FromBrowserContext(browser_context);
  if (!profile) {
    return false;
  }
  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(profile);
  if (!brave_wallet_service) {
    return true;
  }
  // Support to unlock Wallet has been extended also through WebUI,
  // so we block only when Wallet hasn't been created yet, as onboarding
  // is offered only via native Andrioid UI.
  return !brave_wallet_service->keyring_service()->IsWalletCreatedSync();
}
#endif  // BUILDFLAG(IS_ANDROID)
}  // namespace

WebUI::TypeID BraveWebUIControllerFactory::GetWebUIType(
    content::BrowserContext* browser_context,
    const GURL& url) {
  if (brave_rewards::ShouldBlockRewardsWebUI(browser_context, url)) {
    return WebUI::kNoWebUI;
  }
#if BUILDFLAG(IS_ANDROID)
  if (ShouldBlockWalletWebUI(browser_context, url)) {
    return WebUI::kNoWebUI;
  }
#endif  // BUILDFLAG(IS_ANDROID)
#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    if (playlist::PlaylistUI::ShouldBlockPlaylistWebUI(browser_context, url)) {
      return WebUI::kNoWebUI;
    }
  }
#endif

  // Early return to prevent upstream create its WebUI.
  if (url.host_piece() == optimization_guide_internals::
                              kChromeUIOptimizationGuideInternalsHost &&
      !optimization_guide::features::IsOptimizationHintsEnabled()) {
    return WebUI::kNoWebUI;
  }

  Profile* profile = Profile::FromBrowserContext(browser_context);
  WebUIFactoryFunction function =
      GetWebUIFactoryFunction(nullptr, profile, url);
  if (function) {
    return reinterpret_cast<WebUI::TypeID>(function);
  }
  return ChromeWebUIControllerFactory::GetWebUIType(browser_context, url);
}

std::unique_ptr<WebUIController>
BraveWebUIControllerFactory::CreateWebUIControllerForURL(WebUI* web_ui,
                                                         const GURL& url) {
  Profile* profile = Profile::FromWebUI(web_ui);
  WebUIFactoryFunction function = GetWebUIFactoryFunction(web_ui, profile, url);
  if (!function) {
    return ChromeWebUIControllerFactory::CreateWebUIControllerForURL(web_ui,
                                                                     url);
  }

  return base::WrapUnique((*function)(web_ui, url));
}

// static
BraveWebUIControllerFactory* BraveWebUIControllerFactory::GetInstance() {
  static base::NoDestructor<BraveWebUIControllerFactory> instance;
  return instance.get();
}

BraveWebUIControllerFactory::BraveWebUIControllerFactory() = default;

BraveWebUIControllerFactory::~BraveWebUIControllerFactory() = default;
