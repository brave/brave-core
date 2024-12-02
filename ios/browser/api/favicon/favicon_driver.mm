/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/favicon/favicon_driver.h"

#include <optional>

#import "base/values.h"
#import "brave/ios/browser/api/web/web_state/web_state+private.h"
#import "brave/ios/browser/favicon/brave_ios_web_favicon_driver.h"
#include "components/favicon/core/favicon_driver_observer.h"
#include "components/favicon/core/favicon_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "ios/chrome/browser/favicon/model/favicon_service_factory.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_manager_ios.h"
#include "ios/web/favicon/favicon_util.h"
#include "ios/web/js_messaging/web_view_js_utils.h"
#import "ios/web/navigation/navigation_context_impl.h"
#include "ios/web/public/js_messaging/script_message.h"
#import "ios/web/web_state/web_state_impl.h"
#import "net/base/apple/url_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// MARK: - Observers

class FaviconDriverObserver final : public favicon::FaviconDriverObserver {
 public:
  FaviconDriverObserver(
      base::RepeatingCallback<void(const GURL& icon_url,
                                   const gfx::Image& image)> callback);
  ~FaviconDriverObserver() override;

 protected:
  void OnFaviconUpdated(favicon::FaviconDriver* favicon_driver,
                        NotificationIconType notification_icon_type,
                        const GURL& icon_url,
                        bool icon_url_changed,
                        const gfx::Image& image) override;

 private:
  base::RepeatingCallback<void(const GURL& icon_url, const gfx::Image& image)>
      callback_;
};

FaviconDriverObserver::FaviconDriverObserver(
    base::RepeatingCallback<void(const GURL& icon_url, const gfx::Image& image)>
        callback)
    : callback_(callback) {}

FaviconDriverObserver::~FaviconDriverObserver() = default;

void FaviconDriverObserver::OnFaviconUpdated(
    favicon::FaviconDriver* favicon_driver,
    NotificationIconType notification_icon_type,
    const GURL& icon_url,
    bool icon_url_changed,
    const gfx::Image& image) {
  callback_.Run(icon_url, image);
}

// MARK: - Implementation

@interface FaviconDriver () {
  std::unique_ptr<FaviconDriverObserver> driver_observer_;
}
@property(nonatomic, strong, readonly) WebState* webState;
@end

@implementation FaviconDriver
- (instancetype)initWithWebState:(WebState*)webState {
  if ((self = [super init])) {
    _webState = webState;

    web::WebState* real_web_state = [webState internalWebState].get();
    DCHECK(real_web_state);

    ProfileIOS* original_profile =
        ProfileIOS::FromBrowserState(real_web_state->GetBrowserState());

    brave_favicon::BraveIOSWebFaviconDriver::CreateForWebState(
        real_web_state,
        ios::FaviconServiceFactory::GetForProfile(
            original_profile, ServiceAccessType::EXPLICIT_ACCESS));
  }
  return self;
}

- (void)dealloc {
  web::WebState* real_web_state = [self.webState internalWebState].get();
  DCHECK(real_web_state);

  brave_favicon::BraveIOSWebFaviconDriver* driver =
      brave_favicon::BraveIOSWebFaviconDriver::FromWebState(real_web_state);
  DCHECK(driver);

  if (driver_observer_) {
    driver->RemoveObserver(driver_observer_.get());
    driver_observer_ = nullptr;
  }
}

- (void)setMaximumFaviconImageSize:(CGSize)maxImageSize {
  web::WebState* real_web_state = [self.webState internalWebState].get();
  DCHECK(real_web_state);

  brave_favicon::BraveIOSWebFaviconDriver* driver =
      brave_favicon::BraveIOSWebFaviconDriver::FromWebState(real_web_state);
  DCHECK(driver);
  driver->SetMaximumFaviconImageSize(maxImageSize.width, maxImageSize.height);
}

- (void)webView:(WKWebView*)webView
       scriptMessage:(WKScriptMessage*)scriptMessage
    onFaviconUpdated:(void (^)(NSURL*, UIImage*))callback {
  web::WebState* real_web_state = [self.webState internalWebState].get();
  DCHECK(real_web_state);

  brave_favicon::BraveIOSWebFaviconDriver* driver =
      brave_favicon::BraveIOSWebFaviconDriver::FromWebState(real_web_state);
  DCHECK(driver);

  if (driver_observer_) {
    driver->RemoveObserver(driver_observer_.get());
    driver_observer_ = nullptr;
  }

  NSURL* ns_url = scriptMessage.frameInfo.request.URL;
  std::optional<GURL> url;
  if (ns_url) {
    url = net::GURLWithNSURL(ns_url);
  }

  web::ScriptMessage message =
      web::ScriptMessage(web::ValueResultFromWKResult(scriptMessage.body),
                         false, scriptMessage.frameInfo.mainFrame, url);

  DCHECK(message.is_main_frame());
  if (!message.body() || !message.body()->is_list() || !message.request_url()) {
    return;
  }

  const GURL message_request_url = message.request_url().value();

  std::vector<web::FaviconURL> urls;
  if (!ExtractFaviconURL(message.body()->GetList(), message_request_url,
                         &urls)) {
    return;
  }

  if (!urls.empty()) {
    driver_observer_ = std::make_unique<FaviconDriverObserver>(
        base::BindRepeating(^(const GURL& icon_url, const gfx::Image& icon) {
          callback(net::NSURLWithGURL(icon_url),
                   icon.IsEmpty() ? nullptr : icon.ToUIImage());
        }));
    driver->AddObserver(driver_observer_.get());

    web::WebState* web_state = [self.webState internalWebState].get();
    DCHECK(web_state);

    std::unique_ptr<web::NavigationContextImpl> context =
        web::NavigationContextImpl::CreateNavigationContext(
            static_cast<web::WebStateImpl*>(web_state), message_request_url,
            /*has_user_gesture=*/false, ui::PAGE_TRANSITION_TYPED,
            /*is_renderer_initiated=*/true);

    static_cast<web::WebStateImpl*>(web_state)->OnNavigationStarted(
        context.get());
    static_cast<web::WebStateImpl*>(web_state)->OnNavigationFinished(
        context.get());
    static_cast<web::WebStateImpl*>(web_state)->OnFaviconUrlUpdated(urls);
  }
}
@end
