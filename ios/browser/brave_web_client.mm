/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/brave_web_client.h"

#include "base/functional/bind.h"
#include "brave/components/constants/url_constants.h"
#include "brave/ios/browser/brave_web_main_parts.h"
#include "ios/chrome/browser/shared/model/url/chrome_url_constants.h"
#include "ios/components/webui/web_ui_url_constants.h"
#import "ios/public/provider/chrome/browser/url_rewriters/url_rewriters_api.h"
#import "ios/web/public/navigation/browser_url_rewriter.h"
#include "url/gurl.h"

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
  return ChromeWebClient::IsAppSpecificURL(url) || url.SchemeIs(kBraveUIScheme);
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
  // We don't use Chromium web views for anything but WebUI at the moment, so
  // we don't need any JS features added.
  return {};
}

void BraveWebClient::PostBrowserURLRewriterCreation(
    web::BrowserURLRewriter* rewriter) {
  rewriter->AddURLRewriter(&WillHandleBraveURLRedirect);
  ChromeWebClient::PostBrowserURLRewriterCreation(rewriter);
}
