// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_web_ui_controller_factory.h"

#include <memory>

#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/ios/browser/ui/webui/ads/ads_internals_ui.h"
#include "brave/ios/browser/ui/webui/brave_account/brave_account_ui.h"
#include "brave/ios/browser/ui/webui/brave_wallet/line_chart_ui.h"
#include "brave/ios/browser/ui/webui/brave_wallet/market_ui.h"
#include "brave/ios/browser/ui/webui/brave_wallet/nft_ui.h"
#include "brave/ios/browser/ui/webui/brave_wallet/wallet_page_ui.h"
#include "brave/ios/browser/ui/webui/skus/skus_internals_ui.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/url/chrome_url_constants.h"
#include "ios/components/webui/web_ui_url_constants.h"
#include "url/gurl.h"

using web::WebUIIOS;
using web::WebUIIOSController;

namespace brave {
const char kChromeUIUntrustedScheme[] = "chrome-untrusted";

bool ShouldBlockWalletWebUI(ProfileIOS* profile, const GURL& url) {
  if (!url.is_valid() || url.host() != kWalletPageHost) {
    return false;
  }

  if (!profile) {
    return false;
  }

  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForState(profile);
  if (!brave_wallet_service) {
    return true;
  }

  // Support to unlock Wallet has been extended also through WebUI,
  // so we block only when Wallet hasn't been created yet, as onboarding
  // is offered only via native Andrioid UI.
  return !brave_wallet_service->keyring_service()->IsWalletCreatedSync();
}

// A function for creating a new WebUIIOS.
using WebUIIOSFactoryFunction =
    std::unique_ptr<WebUIIOSController> (*)(WebUIIOS* web_ui, const GURL& url);

// Template for defining WebUIIOSFactoryFunction.
template <class T>
std::unique_ptr<WebUIIOSController> NewWebUIIOS(WebUIIOS* web_ui,
                                                const GURL& url) {
  return std::make_unique<T>(web_ui, url);
}

WebUIIOSFactoryFunction GetUntrustedWebUIIOSFactoryFunction(const GURL& url) {
  if (!url.SchemeIs(kChromeUIUntrustedScheme)) {
    return nullptr;
  }

  const std::string url_host = url.host();

  if (url_host == kUntrustedNftHost) {
    return &NewWebUIIOS<nft::UntrustedNftUI>;
  } else if (url_host == kUntrustedMarketHost) {
    return &NewWebUIIOS<market::UntrustedMarketUI>;
  } else if (url_host == kUntrustedLineChartHost) {
    return &NewWebUIIOS<line_chart::UntrustedLineChartUI>;
  }

  return nullptr;
}

// Returns a function that can be used to create the right type of WebUIIOS for
// a tab, based on its URL. Returns nullptr if the URL doesn't have WebUIIOS
// associated with it.
WebUIIOSFactoryFunction GetWebUIIOSFactoryFunction(const GURL& url) {
  // This will get called a lot to check all URLs, so do a quick check of other
  // schemes to filter out most URLs.
  if (!url.SchemeIs(kChromeUIScheme)) {
    return GetUntrustedWebUIIOSFactoryFunction(url);
  }

  const std::string url_host = url.host();

  if (url_host == kAdsInternalsHost) {
    return &NewWebUIIOS<AdsInternalsUI>;
  } else if (url_host == kBraveAccountHost &&
             brave_account::features::IsBraveAccountEnabled()) {
    return &NewWebUIIOS<BraveAccountUI>;
  } else if (url_host == kSkusInternalsHost) {
    return &NewWebUIIOS<SkusInternalsUI>;
  } else if (url_host == kWalletPageHost) {
    return &NewWebUIIOS<WalletPageUI>;
  }
  return nullptr;
}

}  // namespace brave

NSInteger BraveWebUIControllerFactory::GetErrorCodeForWebUIURL(
    const GURL& url) const {
  if (url.host() == kChromeUIDinoHost) {
    return NSURLErrorNotConnectedToInternet;
  }

  if (brave::GetWebUIIOSFactoryFunction(url)) {
    return 0;
  }

  return ChromeWebUIIOSControllerFactory::GetErrorCodeForWebUIURL(url);
}

std::unique_ptr<WebUIIOSController>
BraveWebUIControllerFactory::CreateWebUIIOSControllerForURL(
    WebUIIOS* web_ui,
    const GURL& url) const {
  brave::WebUIIOSFactoryFunction function =
      brave::GetWebUIIOSFactoryFunction(url);
  if (!function) {
    return ChromeWebUIIOSControllerFactory::CreateWebUIIOSControllerForURL(
        web_ui, url);
  }

  if (brave::ShouldBlockWalletWebUI(ProfileIOS::FromWebUIIOS(web_ui), url)) {
    return nullptr;
  }

  return (*function)(web_ui, url);
}

// static
BraveWebUIControllerFactory* BraveWebUIControllerFactory::GetInstance() {
  static base::NoDestructor<BraveWebUIControllerFactory> instance;
  return instance.get();
}

BraveWebUIControllerFactory::BraveWebUIControllerFactory() = default;

BraveWebUIControllerFactory::~BraveWebUIControllerFactory() = default;
