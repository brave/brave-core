// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_web_ui_controller_factory.h"

#include <memory>

#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/ios/browser/ui/webui/ads/ads_internals_ui.h"
#include "brave/ios/browser/ui/webui/skus/skus_internals_ui.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/url/chrome_url_constants.h"
#include "ios/components/webui/web_ui_url_constants.h"
#include "url/gurl.h"

using web::WebUIIOS;
using web::WebUIIOSController;

namespace brave {

// A function for creating a new WebUIIOS.
using WebUIIOSFactoryFunction =
    std::unique_ptr<WebUIIOSController> (*)(WebUIIOS* web_ui, const GURL& url);

// Template for defining WebUIIOSFactoryFunction.
template <class T>
std::unique_ptr<WebUIIOSController> NewWebUIIOS(WebUIIOS* web_ui,
                                                const GURL& url) {
  return std::make_unique<T>(web_ui, url);
}

// Returns a function that can be used to create the right type of WebUIIOS for
// a tab, based on its URL. Returns nullptr if the URL doesn't have WebUIIOS
// associated with it.
WebUIIOSFactoryFunction GetWebUIIOSFactoryFunction(const GURL& url) {
  // This will get called a lot to check all URLs, so do a quick check of other
  // schemes to filter out most URLs.
  if (!url.SchemeIs(kBraveUIScheme) && !url.SchemeIs(kChromeUIScheme)) {
    return nullptr;
  }

  // TODO: Handle RewardsInternalUI, AdsInternalUI, AdblockInternalUI URLs here
  // ProfileIOS* browser_state =
  // ProfileIOS::FromWebUIIOS(web_ui);

  const std::string url_host = url.host();
  /*if (url_host == kAdblockInternalsHost) {
    return &NewWebUIIOS<BraveAdblockInternalsUI>;
  } if (url_host == kRewardsInternalsHost &&
             brave_rewards::IsSupportedForProfile(browser_state)) {
    return &NewWebUIIOS<BraveRewardsInternalsUI>;
  }*/

  if (url_host == kAdsInternalsHost) {
    return &NewWebUIIOS<AdsInternalsUI>;
  } else if (url_host == kSkusInternalsHost) {
    return &NewWebUIIOS<SkusInternalsUI>;
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

  return (*function)(web_ui, url);
}

// static
BraveWebUIControllerFactory* BraveWebUIControllerFactory::GetInstance() {
  static base::NoDestructor<BraveWebUIControllerFactory> instance;
  return instance.get();
}

BraveWebUIControllerFactory::BraveWebUIControllerFactory() = default;

BraveWebUIControllerFactory::~BraveWebUIControllerFactory() = default;
