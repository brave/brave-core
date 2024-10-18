// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import "brave/ios/browser/web/brave_web_client.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/ios/ns_error_util.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/constants/url_constants.h"
#include "brave/ios/browser/web/brave_web_main_parts.h"
#import "components/translate/ios/browser/translate_java_script_feature.h"
#include "ios/chrome/browser/shared/model/url/chrome_url_constants.h"
#include "ios/chrome/browser/web/model/chrome_web_client.h"
#import "ios/components/security_interstitials/ios_security_interstitial_java_script_feature.h"
#import "ios/components/security_interstitials/lookalikes/lookalike_url_error.h"
#import "ios/components/security_interstitials/safe_browsing/safe_browsing_error.h"
#include "ios/components/webui/web_ui_url_constants.h"
#import "ios/public/provider/chrome/browser/url_rewriters/url_rewriters_api.h"
#import "ios/web/public/navigation/browser_url_rewriter.h"
#import "ios/web_view/internal/cwv_ssl_error_handler_internal.h"
#import "ios/web_view/internal/cwv_web_view_internal.h"
#import "ios/web_view/public/cwv_navigation_delegate.h"
#import "net/base/apple/url_conversions.h"
#include "url/gurl.h"

BraveWebClient::BraveWebClient() {}

BraveWebClient::~BraveWebClient() {}

std::unique_ptr<web::WebMainParts> BraveWebClient::CreateWebMainParts() {
  return std::make_unique<BraveWebMainParts>(
      *base::CommandLine::ForCurrentProcess());
}

std::string BraveWebClient::GetUserAgent(web::UserAgentType type) const {
  // FIXME: Decide on whether or not to adjust the product type (defaults to
  // CriOS/<version>)
  return ChromeWebClient::GetUserAgent(type);
}

web::UserAgentType BraveWebClient::GetDefaultUserAgent(web::WebState* web_state,
                                                       const GURL& url) const {
  // FIXME: Add a way to force desktop mode always via prefs
  // FIXME: Possibly handle desktop by default on iPad here?
  return ChromeWebClient::GetDefaultUserAgent(web_state, url);
}

void BraveWebClient::AddAdditionalSchemes(Schemes* schemes) const {
  ChromeWebClient::AddAdditionalSchemes(schemes);

  schemes->standard_schemes.push_back(kBraveUIScheme);
  schemes->secure_schemes.push_back(kBraveUIScheme);
}

bool BraveWebClient::IsAppSpecificURL(const GURL& url) const {
  // temporarily add `internal://` scheme handling until those pages can be
  // ported to WebUI
  return ChromeWebClient::IsAppSpecificURL(url) ||
         url.SchemeIs(kBraveUIScheme) || url.SchemeIs("internal");
}

bool WillHandleBraveURLRedirect(GURL* url, web::BrowserState* browser_state) {
  if (url->SchemeIs(kBraveUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(kChromeUIScheme);
    *url = url->ReplaceComponents(replacements);
  }
  return false;
}

std::vector<web::JavaScriptFeature*> BraveWebClient::GetJavaScriptFeatures(
    web::BrowserState* browser_state) const {
  // Disable majority of ChromeWebClient JS features
  std::vector<web::JavaScriptFeature*> features;
  // FIXME: Add any JavaScriptFeature's from Chromium as needed
  features.push_back(
      security_interstitials::IOSSecurityInterstitialJavaScriptFeature::
          GetInstance());
  features.push_back(translate::TranslateJavaScriptFeature::GetInstance());
  return features;
}

void BraveWebClient::PostBrowserURLRewriterCreation(
    web::BrowserURLRewriter* rewriter) {
  rewriter->AddURLRewriter(&WillHandleBraveURLRedirect);
  ChromeWebClient::PostBrowserURLRewriterCreation(rewriter);
}

bool BraveWebClient::EnableLongPressUIContextMenu() const {
  return CWVWebView.chromeContextMenuEnabled;
}

bool BraveWebClient::EnableWebInspector(
    web::BrowserState* browser_state) const {
  // FIXME: Probably better to just use prefs::kWebInspectorEnabled
  return CWVWebView.webInspectorEnabled;
}
