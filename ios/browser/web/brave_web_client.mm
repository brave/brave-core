// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import "brave/ios/browser/web/brave_web_client.h"

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/ios/ns_error_util.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/constants/url_constants.h"
#include "brave/ios/browser/api/web_view/brave_web_view_internal.h"
#include "brave/ios/browser/ui/web_view/features.h"
#include "brave/ios/browser/web/brave_web_main_parts.h"
#include "components/autofill/ios/browser/autofill_java_script_feature.h"
#include "components/autofill/ios/browser/suggestion_controller_java_script_feature.h"
#include "components/autofill/ios/form_util/form_handlers_java_script_feature.h"
#include "components/password_manager/ios/password_manager_java_script_feature.h"
#import "components/translate/ios/browser/translate_java_script_feature.h"
#include "ios/chrome/browser/shared/model/url/chrome_url_constants.h"
#include "ios/chrome/browser/web/model/chrome_web_client.h"
#import "ios/components/security_interstitials/ios_security_interstitial_java_script_feature.h"
#import "ios/components/security_interstitials/lookalikes/lookalike_url_error.h"
#import "ios/components/security_interstitials/safe_browsing/safe_browsing_error.h"
#include "ios/components/webui/web_ui_url_constants.h"
#import "ios/public/provider/chrome/browser/url_rewriters/url_rewriters_api.h"
#include "ios/web/common/url_scheme_util.h"
#include "ios/web/common/user_agent.h"
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
  if (!legacy_user_agent_.empty()) {
    return legacy_user_agent_;
  }
  return ChromeWebClient::GetUserAgent(type);
}

void BraveWebClient::AddAdditionalSchemes(Schemes* schemes) const {
  ChromeWebClient::AddAdditionalSchemes(schemes);

  schemes->standard_schemes.push_back(kBraveUIScheme);
  schemes->secure_schemes.push_back(kBraveUIScheme);

  schemes->standard_schemes.push_back(kChromeUIUntrustedScheme);
  schemes->secure_schemes.push_back(kChromeUIUntrustedScheme);
}

bool BraveWebClient::IsAppSpecificURL(const GURL& url) const {
  // temporarily add `internal://` scheme handling until those pages can be
  // ported to WebUI
  return ChromeWebClient::IsAppSpecificURL(url) ||
         url.SchemeIs(kBraveUIScheme) ||
         url.SchemeIs(kChromeUIUntrustedScheme) || url.SchemeIs("internal");
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
  // Add any JavaScriptFeature's from Chromium or Brave as needed
  std::vector<web::JavaScriptFeature*> features;
  features.push_back(
      security_interstitials::IOSSecurityInterstitialJavaScriptFeature::
          GetInstance());
  if (base::FeatureList::IsEnabled(
          brave::features::kUseChromiumWebViewsAutofill)) {
    features.push_back(
        password_manager::PasswordManagerJavaScriptFeature::GetInstance());
    features.push_back(autofill::AutofillJavaScriptFeature::GetInstance());
    features.push_back(autofill::FormHandlersJavaScriptFeature::GetInstance());
    features.push_back(
        autofill::SuggestionControllerJavaScriptFeature::GetInstance());
  }
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
  return CWVWebView.webInspectorEnabled;
}

void BraveWebClient::SetLegacyUserAgent(const std::string& user_agent) {
  legacy_user_agent_ = user_agent;
}

bool BraveWebClient::IsInsecureFormWarningEnabled(
    web::BrowserState* browser_state) const {
  // TODO: Remove when brave/brave-browser#46667 is implemented
  return false;
}

void BraveWebClient::BuildEditMenu(web::WebState* web_state,
                                   id<UIMenuBuilder> builder) const {
  BraveWebView* webView = [BraveWebView braveWebViewForWebState:web_state];
  if (!webView) {
    return;
  }
  id<BraveWebViewUIDelegate> uiDelegate = webView.UIDelegate;

  if ([uiDelegate respondsToSelector:@selector(webView:
                                         buildEditMenuWithBuilder:)]) {
    return [uiDelegate webView:webView buildEditMenuWithBuilder:builder];
  }
}

bool BraveWebClient::ShouldBlockJavaScript(web::WebState* webState,
                                           NSURLRequest* request) {
  if (!web::UrlHasWebScheme(request.URL)) {
    return false;
  }
  BraveWebView* webView = [BraveWebView braveWebViewForWebState:webState];
  if (!webView) {
    return false;
  }
  id<BraveWebViewNavigationDelegate> navigationDelegate =
      webView.navigationDelegate;

  if ([navigationDelegate respondsToSelector:@selector
                          (webView:shouldBlockJavaScriptForRequest:)]) {
    return [navigationDelegate webView:webView
        shouldBlockJavaScriptForRequest:request];
  }
  return false;
}

bool BraveWebClient::ShouldBlockUniversalLinks(web::WebState* webState,
                                               NSURLRequest* request) {
  BraveWebView* webView = [BraveWebView braveWebViewForWebState:webState];
  if (!webView) {
    return false;
  }
  id<BraveWebViewNavigationDelegate> navigationDelegate =
      webView.navigationDelegate;

  if ([navigationDelegate respondsToSelector:@selector
                          (webView:shouldBlockUniversalLinksForRequest:)]) {
    return [navigationDelegate webView:webView
        shouldBlockUniversalLinksForRequest:request];
  }
  return false;
}

NSString* BraveWebClient::GetUserAgentForRequest(
    web::WebState* webState,
    web::UserAgentType userAgentType,
    NSURLRequest* request) {
  BraveWebView* webView = [BraveWebView braveWebViewForWebState:webState];
  if (!webView) {
    return nil;
  }

  id<BraveWebViewNavigationDelegate> navigationDelegate =
      webView.navigationDelegate;
  if ([navigationDelegate respondsToSelector:@selector
                          (webView:userAgentForUserAgentType:request:)]) {
    return [navigationDelegate
                          webView:webView
        userAgentForUserAgentType:static_cast<CWVUserAgentType>(userAgentType)
                          request:request];
  }
  return nil;
}
