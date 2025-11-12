// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/web_view/brave_web_view.h"

#include "base/notreached.h"
#include "ios/chrome/browser/tabs/model/tab_helper_util.h"
#include "ios/web/public/navigation/web_state_policy_decider.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/web_state_user_data.h"
#include "ios/web_view/internal/cwv_navigation_action_internal.h"
#include "ios/web_view/internal/cwv_navigation_type_internal.h"
#include "ios/web_view/internal/cwv_web_view_internal.h"

@interface BraveNavigationAction ()
- (instancetype)initWithRequest:(NSURLRequest*)request
                    requestInfo:
                        (web::WebStatePolicyDecider::RequestInfo)requestInfo;
@end

namespace {

class BraveWebViewWebStatePolicyDecider : public web::WebStatePolicyDecider {
 public:
  BraveWebViewWebStatePolicyDecider(web::WebState* web_state,
                                    BraveWebView* web_view)
      : web::WebStatePolicyDecider(web_state), web_view_(web_view) {}

  // web::WebStatePolicyDecider overrides:
  void ShouldAllowRequest(
      NSURLRequest* request,
      web::WebStatePolicyDecider::RequestInfo request_info,
      web::WebStatePolicyDecider::PolicyDecisionCallback callback) override {
    id<BraveWebViewNavigationDelegate> delegate = web_view_.navigationDelegate;
    if ([delegate respondsToSelector:@selector
                  (webView:
                      decidePolicyForBraveNavigationAction:decisionHandler:)]) {
      BraveNavigationAction* navigationAction =
          [[BraveNavigationAction alloc] initWithRequest:request
                                             requestInfo:request_info];

      __block web::WebStatePolicyDecider::PolicyDecisionCallback
          block_callback = std::move(callback);
      [delegate webView:web_view_
          decidePolicyForBraveNavigationAction:navigationAction
                               decisionHandler:^(
                                   CWVNavigationActionPolicy policy) {
                                 switch (policy) {
                                   case CWVNavigationActionPolicyCancel:
                                     std::move(block_callback)
                                         .Run(web::WebStatePolicyDecider::
                                                  PolicyDecision::Cancel());
                                     break;
                                   case CWVNavigationActionPolicyAllow:
                                     std::move(block_callback)
                                         .Run(web::WebStatePolicyDecider::
                                                  PolicyDecision::Allow());
                                     break;
                                 }
                               }];
      return;
    }
    std::move(callback).Run(
        web::WebStatePolicyDecider::PolicyDecision::Allow());
  }

 private:
  // Delegates to |delegate| property of this web view.
  __weak BraveWebView* web_view_ = nil;
};

// A WebStateUserData to hold a reference to a corresponding BraveWebView.
class BraveWebViewHolder : public web::WebStateUserData<BraveWebViewHolder> {
 public:
  explicit BraveWebViewHolder(web::WebState* web_state, BraveWebView* web_view)
      : web_view_(web_view) {}
  BraveWebView* web_view() const { return web_view_; }

 private:
  friend class web::WebStateUserData<BraveWebViewHolder>;

  __weak BraveWebView* web_view_ = nil;
};

}  // namespace

@implementation BraveNavigationAction
- (instancetype)initWithRequest:(NSURLRequest*)request
                    requestInfo:
                        (web::WebStatePolicyDecider::RequestInfo)requestInfo {
  if ((self = [super initWithRequest:request
                       userInitiated:requestInfo.is_user_initiated
                      navigationType:CWVNavigationTypeFromPageTransition(
                                         requestInfo.transition_type)])) {
    _targetFrameIsMain = requestInfo.target_frame_is_main;
    _targetFrameIsCrossOrigin = requestInfo.target_frame_is_cross_origin;
    _targetWindowIsCrossOrigin = requestInfo.target_window_is_cross_origin;
    _hasTappedRecently = requestInfo.user_tapped_recently;
  }
  return self;
}
@end

@interface CWVWebView ()
@property(nonatomic, readwrite) BOOL loading;
- (void)resetWebStateWithCoder:(NSCoder*)coder
               WKConfiguration:(WKWebViewConfiguration*)wkConfiguration
                createdWebView:(WKWebView**)createdWebView;
- (void)attachSecurityInterstitialHelpersToWebStateIfNecessary;
- (void)updateCurrentURLs;
- (void)updateVisibleSSLStatus;
- (void)updateNavigationAvailability;
@end

@implementation BraveWebView {
  std::unique_ptr<BraveWebViewWebStatePolicyDecider> _webStatePolicyDecider;
}

// These are shadowed CWVWebView properties
@dynamic navigationDelegate, UIDelegate;

- (void)dealloc {
  if (self.webState) {
    BraveWebViewHolder::RemoveFromWebState(self.webState);
  }
}

+ (nullable BraveWebView*)braveWebViewForWebState:(web::WebState*)webState {
  if (!webState || webState->IsBeingDestroyed()) {
    // Check web state for safety
    return nil;
  }
  BraveWebViewHolder* holder = BraveWebViewHolder::FromWebState(webState);
  if (!holder) {
    // The holder may have already been destroyed if the web view is deallocated
    // or the web state was reset on the web view itself, even if the underlying
    // WebState is alive
    return nil;
  }
  return holder->web_view();
}

- (void)resetWebStateWithCoder:(NSCoder*)coder
               WKConfiguration:(WKWebViewConfiguration*)wkConfiguration
                createdWebView:(WKWebView**)createdWebView {
  if (self.webState) {
    BraveWebViewHolder::RemoveFromWebState(self.webState);
  }

  [super resetWebStateWithCoder:coder
                WKConfiguration:wkConfiguration
                 createdWebView:createdWebView];

  BraveWebViewHolder::CreateForWebState(self.webState, /*web_view=*/self);

  _webStatePolicyDecider =
      std::make_unique<BraveWebViewWebStatePolicyDecider>(self.webState, self);
}

- (void)attachSecurityInterstitialHelpersToWebStateIfNecessary {
  [super attachSecurityInterstitialHelpersToWebStateIfNecessary];
  AttachTabHelpers(self.webState);
}

- (void)updateForOnDownloadCreated {
  // There is a bug in Chromium where OnNavigationFinished is not called when a
  // root navigation turns into a download, this workaround ensures that the
  // info typically updated in that observer method are updated when a download
  // task is created.
  if (!self.webState || self.webState->IsBeingDestroyed()) {
    return;
  }
  [self updateNavigationAvailability];
  [self updateCurrentURLs];
  [self updateVisibleSSLStatus];
  self.loading = self.webState->IsLoading();
}

- (CWVAutofillController*)autofillController {
  NOTREACHED();
  return nil;
}

- (CWVTranslationController*)translationController {
  NOTREACHED();
  return nil;
}

#pragma mark - CRWWebStateDelegate

- (void)webState:(web::WebState*)webState
    didRequestHTTPAuthForProtectionSpace:(NSURLProtectionSpace*)protectionSpace
                      proposedCredential:(NSURLCredential*)proposedCredential
                       completionHandler:(void (^)(NSString* username,
                                                   NSString* password))handler {
  SEL selector = @selector(webView:
      didRequestHTTPAuthForProtectionSpace:proposedCredential:completionHandler
                                          :);
  if ([self.navigationDelegate respondsToSelector:selector]) {
    [self.navigationDelegate webView:self
        didRequestHTTPAuthForProtectionSpace:protectionSpace
                          proposedCredential:proposedCredential
                           completionHandler:handler];
  } else {
    handler(nil, nil);
  }
}

- (void)webStateDidCreateWebView:(web::WebState*)webState {
  SEL selector = @selector(webViewDidCreateNewWebView:);
  if ([self.UIDelegate respondsToSelector:selector]) {
    [self.UIDelegate webViewDidCreateNewWebView:self];
  }
}

#pragma mark - CRWWebStateObserver

- (void)webState:(web::WebState*)webState
    didRedirectNavigation:(web::NavigationContext*)navigationContext {
  [self updateCurrentURLs];
  if ([self.navigationDelegate
          respondsToSelector:@selector(webViewDidRedirectNavigation:)]) {
    [self.navigationDelegate webViewDidRedirectNavigation:self];
  }
}

@end
