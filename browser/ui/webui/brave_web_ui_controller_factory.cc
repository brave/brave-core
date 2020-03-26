/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_web_ui_controller_factory.h"

#include <memory>

#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "brave/browser/ui/webui/brave_adblock_ui.h"
#include "brave/browser/ui/webui/webcompat_reporter_ui.h"
#include "brave/browser/ui/webui/brave_new_tab_ui.h"
#include "brave/common/brave_features.h"
#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_sync/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

#if !defined(OS_ANDROID)
#include "brave/browser/ui/webui/brave_settings_ui.h"
#include "brave/browser/ui/webui/brave_welcome_ui.h"
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/browser/ui/webui/brave_tip_ui.h"
#include "brave/browser/ui/webui/brave_checkout_ui.h"
#include "brave/browser/ui/webui/brave_rewards_internals_ui.h"
#include "brave/browser/ui/webui/brave_rewards_page_ui.h"
#endif

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
#include "brave/browser/ui/webui/brave_wallet_ui.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_SYNC)
#include "brave/browser/ui/webui/sync/sync_ui.h"
#include "components/sync/driver/sync_driver_switches.h"
#endif

using content::WebUI;
using content::WebUIController;

namespace {

// A function for creating a new WebUI. The caller owns the return value, which
// may be NULL (for example, if the URL refers to an non-existent extension).
typedef WebUIController* (*WebUIFactoryFunction)(WebUI* web_ui,
                                                 const GURL& url);

// Template for defining WebUIFactoryFunction.
template<class T>
WebUIController* NewWebUI(WebUI* web_ui, const GURL& url) {
  return new T(web_ui);
}

template<>
WebUIController* NewWebUI<BasicUI>(WebUI* web_ui, const GURL& url) {
  auto host = url.host_piece();
  if (host == kAdblockHost) {
    return new BraveAdblockUI(web_ui, url.host());
  } else if (host == kWebcompatReporterHost) {
    return new WebcompatReporterUI(web_ui, url.host());
#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  } else if (host == kWalletHost) {
    return new BraveWalletUI(web_ui, url.host());
#endif  // BUILDFLAG(BRAVE_WALLET_ENABLED)
#if BUILDFLAG(ENABLE_BRAVE_SYNC)
  } else if (host == kBraveUISyncHost &&
             switches::IsSyncAllowedByFlag()) {
    return new SyncUI(web_ui, url.host());
#endif  // BUILDFLAG(ENABLE_BRAVE_SYNC)
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  } else if (host == kRewardsPageHost) {
    return new BraveRewardsPageUI(web_ui, url.host());
  } else if (host == kRewardsInternalsHost) {
    return new BraveRewardsInternalsUI(web_ui, url.host());
#if !defined(OS_ANDROID)
  } else if (host == kBraveUITipHost) {
    return new BraveTipUI(web_ui, url.host());
  } else if (host == kBraveUICheckoutHost) {
    return new BraveCheckoutUI(web_ui, url.host());
#endif  // !defined(OS_ANDROID)
#endif  // BUILDFLAG(BRAVE_REWARDS_ENABLED)
#if !defined(OS_ANDROID)
  } else if (host == kWelcomeHost) {
    return new BraveWelcomeUI(web_ui, url.host());
  } else if (host == chrome::kChromeUISettingsHost) {
    return new BraveSettingsUI(web_ui, url.host());
#endif  // !defined(OS_ANDROID)
  } else if (host == chrome::kChromeUINewTabHost) {
    return new BraveNewTabUI(web_ui, url.host());
  }
  return nullptr;
}

// Returns a function that can be used to create the right type of WebUI for a
// tab, based on its URL. Returns NULL if the URL doesn't have WebUI associated
// with it.
WebUIFactoryFunction GetWebUIFactoryFunction(WebUI* web_ui,
                                             const GURL& url) {
  if (url.host_piece() == kAdblockHost ||
      url.host_piece() == kWebcompatReporterHost ||
#if BUILDFLAG(BRAVE_WALLET_ENABLED)
      url.host_piece() == kWalletHost ||
#endif
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
      url.host_piece() == kRewardsPageHost ||
      url.host_piece() == kRewardsInternalsHost ||
      url.host_piece() == kBraveUITipHost ||
      url.host_piece() == kBraveUICheckoutHost ||
#endif
      url.host_piece() == kWelcomeHost ||
      url.host_piece() == chrome::kChromeUIWelcomeURL ||
#if BUILDFLAG(ENABLE_BRAVE_SYNC)
      (url.host_piece() == kBraveUISyncHost &&
       switches::IsSyncAllowedByFlag()) ||
#endif
      url.host_piece() == chrome::kChromeUINewTabHost ||
      url.host_piece() == chrome::kChromeUISettingsHost) {
    return &NewWebUI<BasicUI>;
  }

  return nullptr;
}

#if defined(OS_ANDROID)
bool ShouldBlockRewardsWebUI(
      content::BrowserContext* browser_context, const GURL& url) {
  if (url.host_piece() != kRewardsPageHost &&
      url.host_piece() != kRewardsInternalsHost) {
    return false;
  }
  if (!base::FeatureList::IsEnabled(features::kBraveRewards)) {
    return true;
  }
  Profile* profile = Profile::FromBrowserContext(browser_context);
  if (profile &&
      profile->GetPrefs() &&
      profile->GetPrefs()->GetBoolean(kSafetynetCheckFailed)) {
    return true;
  }
  return false;
}
#endif  // defined(OS_ANDROID)

}  // namespace

WebUI::TypeID BraveWebUIControllerFactory::GetWebUIType(
      content::BrowserContext* browser_context, const GURL& url) {
#if defined(OS_ANDROID)
  if (ShouldBlockRewardsWebUI(browser_context, url)) {
    return WebUI::kNoWebUI;
  }
#endif  // defined(OS_ANDROID)
  WebUIFactoryFunction function = GetWebUIFactoryFunction(NULL, url);
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
    return ChromeWebUIControllerFactory::CreateWebUIControllerForURL(
        web_ui, url);
  }

  return base::WrapUnique((*function)(web_ui, url));
}


// static
BraveWebUIControllerFactory* BraveWebUIControllerFactory::GetInstance() {
  return base::Singleton<BraveWebUIControllerFactory>::get();
}

BraveWebUIControllerFactory::BraveWebUIControllerFactory() {
}

BraveWebUIControllerFactory::~BraveWebUIControllerFactory() {
}
