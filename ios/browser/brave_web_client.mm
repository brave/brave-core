/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/brave_web_client.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/ios/ns_error_util.h"
#include "brave/components/constants/url_constants.h"
#include "brave/ios/browser/brave_web_main_parts.h"
#include "ios/chrome/browser/shared/model/url/chrome_url_constants.h"
#include "ios/components/webui/web_ui_url_constants.h"
#import "ios/public/provider/chrome/browser/url_rewriters/url_rewriters_api.h"
#import "ios/web/public/navigation/browser_url_rewriter.h"
#import "net/base/apple/url_conversions.h"
#include "url/gurl.h"

#import "ios/components/security_interstitials/ios_security_interstitial_java_script_feature.h"
#import "ios/components/security_interstitials/lookalikes/lookalike_url_error.h"
#import "ios/components/security_interstitials/safe_browsing/safe_browsing_error.h"
#import "ios/web_view/internal/cwv_web_view_internal.h"
#import "ios/web_view/public/cwv_navigation_delegate.h"
#import "ios/web_view/internal/cwv_ssl_error_handler_internal.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

BraveWebClient::BraveWebClient() {}

BraveWebClient::~BraveWebClient() {
}

std::unique_ptr<web::WebMainParts> BraveWebClient::CreateWebMainParts() {
  return std::make_unique<BraveWebMainParts>(
      *base::CommandLine::ForCurrentProcess());
}

void BraveWebClient::SetUserAgent(const std::string& user_agent) {
  user_agent_ = user_agent;
}

std::string BraveWebClient::GetUserAgent(web::UserAgentType type) const {
  if (user_agent_.empty()) {
    return ChromeWebClient::GetUserAgent(type);
  }
  return user_agent_;
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
         url.SchemeIs(kBraveUIScheme) ||
         url.SchemeIs("internal");
}

bool WillHandleBraveURLRedirect(GURL* url, web::BrowserState* browser_state) {
  if (url->SchemeIs(kBraveUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(kChromeUIScheme);
    *url = url->ReplaceComponents(replacements);
  }
  return false;
}

std::vector<web::JavaScriptFeature*> BraveWebClient::GetJavaScriptFeatures(web::BrowserState* browser_state) const {
  // Disable majority of ChromeWebClient JS features
  std::vector<web::JavaScriptFeature*> features;
  // FIXME: Add any JavaScriptFeature's from Chromium as needed
  return features;
}

void BraveWebClient::PostBrowserURLRewriterCreation(
    web::BrowserURLRewriter* rewriter) {
  rewriter->AddURLRewriter(&WillHandleBraveURLRedirect);
  ChromeWebClient::PostBrowserURLRewriterCreation(rewriter);
}

void BraveWebClient::PrepareErrorPage(
    web::WebState* web_state,
    const GURL& url,
    NSError* error,
    bool is_post,
    bool is_off_the_record,
    const std::optional<net::SSLInfo>& info,
    int64_t navigation_id,
    base::OnceCallback<void(NSString*)> callback) {
  DCHECK(error);

  CWVWebView* web_view = [CWVWebView webViewForWebState:web_state];
  id<CWVNavigationDelegate> navigation_delegate = web_view.navigationDelegate;

  // |final_underlying_error| should be checked first for any specific error
  // cases such as lookalikes and safebrowsing errors. |info| is only non-empty
  // if this is a SSL related error.
  NSError* final_underlying_error =
      base::ios::GetFinalUnderlyingErrorFromError(error);
  if ([final_underlying_error.domain isEqual:kSafeBrowsingErrorDomain] &&
      [navigation_delegate
          respondsToSelector:@selector(webView:handleUnsafeURLWithHandler:)]) {
    DCHECK_EQ(kUnsafeResourceErrorCode, final_underlying_error.code);
  }
  else if ([final_underlying_error.domain isEqual:kLookalikeUrlErrorDomain] &&
             [navigation_delegate respondsToSelector:@selector
                                  (webView:handleLookalikeURLWithHandler:)]) {
    DCHECK_EQ(kLookalikeUrlErrorCode, final_underlying_error.code);
  }
  else if (info.has_value() &&
             [navigation_delegate respondsToSelector:@selector
                                  (webView:handleSSLErrorWithHandler:)]) {
    CWVSSLErrorHandler* handler = [[CWVSSLErrorHandler alloc]
             initWithWebState:web_state
                          URL:net::NSURLWithGURL(url)
                        error:error
                      SSLInfo:info.value()
        errorPageHTMLCallback:base::CallbackToBlock(std::move(callback))];
    [navigation_delegate webView:web_view handleSSLErrorWithHandler:handler];
  } else {
    std::move(callback).Run(error.localizedDescription);
  }
}

bool BraveWebClient::EnableLongPressUIContextMenu() const {
  return CWVWebView.chromeContextMenuEnabled;
}

bool BraveWebClient::EnableWebInspector(
    web::BrowserState* browser_state) const {
  // FIXME: Probably better to just use prefs::kWebInspectorEnabled
  return CWVWebView.webInspectorEnabled;
}
