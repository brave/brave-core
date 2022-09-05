/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/favicon/favicon_driver.h"
#import "brave/ios/browser/favicon/brave_ios_web_favicon_driver.h"
#include "components/favicon/core/favicon_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "ios/chrome/browser/application_context/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/favicon/favicon_service_factory.h"
#include "ios/web/favicon/favicon_util.h"
#include "ios/web/js_messaging/web_view_js_utils.h"
#include "ios/web/public/js_messaging/script_message.h"
#import "net/base/mac/url_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// MARK: - Implementation

@interface BraveFaviconDriver () {
  ChromeBrowserState* browser_state_;
}
@end

@implementation BraveFaviconDriver
- (instancetype)initWithPrivateBrowsingMode:(bool)privateMode {
  if ((self = [super init])) {
    ios::ChromeBrowserStateManager* browser_state_manager =
        GetApplicationContext()->GetChromeBrowserStateManager();
    CHECK(browser_state_manager);

    browser_state_ = browser_state_manager->GetLastUsedBrowserState();
    CHECK(browser_state_);

    if (privateMode) {
      browser_state_ = browser_state_->GetOffTheRecordChromeBrowserState();
      CHECK(browser_state_);
    }

    brave_favicon::BraveIOSWebFaviconDriver::CreateForBrowserState(
        browser_state_,
        ios::FaviconServiceFactory::GetForBrowserState(
            browser_state_, ServiceAccessType::EXPLICIT_ACCESS));
  }
  return self;
}

- (void)setMaximumFaviconImageSize:(NSUInteger)maxImageSize {
  brave_favicon::BraveIOSWebFaviconDriver* driver =
      brave_favicon::BraveIOSWebFaviconDriver::FromBrowserState(browser_state_);
  DCHECK(driver);
  driver->SetMaximumFaviconImageSize(maxImageSize);
}

- (void)webView:(WKWebView*)webView
    onFaviconURLsUpdated:(WKScriptMessage*)scriptMessage {
  NSURL* ns_url = scriptMessage.frameInfo.request.URL;
  absl::optional<GURL> url;
  if (ns_url) {
    url = net::GURLWithNSURL(ns_url);
  }

  web::ScriptMessage message(web::ValueResultFromWKResult(scriptMessage.body),
                             false, scriptMessage.frameInfo.mainFrame, url);

  const GURL message_request_url = message.request_url().value();

  std::vector<web::FaviconURL> urls;
  if (!ExtractFaviconURL(message.body()->GetListDeprecated(),
                         message_request_url, &urls)) {
    return;
  }

  if (!urls.empty()) {
    brave_favicon::BraveIOSWebFaviconDriver* driver =
        brave_favicon::BraveIOSWebFaviconDriver::FromBrowserState(
            browser_state_);
    DCHECK(driver);

    driver->DidStartNavigation(browser_state_, message_request_url);
    driver->DidFinishNavigation(browser_state_, message_request_url);
    driver->FaviconUrlUpdated(urls);
  }
}
@end
