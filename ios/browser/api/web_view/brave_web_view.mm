// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/web_view/brave_web_view.h"

#include <memory>

#include "base/notreached.h"
#include "brave/components/ai_chat/ios/browser/ai_chat_associated_content_page_fetcher.h"
#include "brave/components/ai_chat/ios/browser/ai_chat_tab_helper.h"
#include "brave/ios/browser/ai_chat/ai_chat_ui_handler_bridge_holder.h"
#include "brave/ios/browser/ai_chat/tab_data_web_state_observer.h"
#include "brave/ios/browser/ai_chat/tab_tracker_service_factory.h"
#include "brave/ios/browser/api/web_view/autofill/brave_web_view_autofill_client.h"
#include "brave/ios/browser/api/web_view/passwords/brave_web_view_password_manager_client.h"
#include "brave/ios/browser/ui/web_view/features.h"
#include "brave/ios/browser/ui/webui/brave_wallet/wallet_page_handler_bridge_holder.h"
#include "components/autofill/core/browser/logging/log_manager.h"
#include "components/autofill/core/browser/logging/log_router.h"
#include "components/autofill/ios/browser/autofill_agent.h"
#include "components/autofill/ios/browser/autofill_client_ios.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/leak_detection/leak_detection_request_utils.h"
#include "components/password_manager/core/browser/password_form.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/password_manager/ios/password_controller_driver_helper.h"
#include "components/password_manager/ios/password_manager_ios_util.h"
#include "components/password_manager/ios/password_suggestion_helper.h"
#include "components/password_manager/ios/shared_password_controller.h"
#include "ios/chrome/browser/autofill/model/autofill_log_router_factory.h"
#include "ios/chrome/browser/autofill/model/personal_data_manager_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_account_password_store_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_profile_password_store_factory.h"
#include "ios/chrome/browser/passwords/model/password_controller.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/signin/model/identity_manager_factory.h"
#include "ios/chrome/browser/sync/model/sync_service_factory.h"
#include "ios/chrome/browser/tabs/model/tab_helper_util.h"
#include "ios/web/common/crw_input_view_provider.h"
#include "ios/web/public/navigation/web_state_policy_decider.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/web_state_user_data.h"
#include "ios/web_view/internal/autofill/cwv_autofill_controller_internal.h"
#include "ios/web_view/internal/cwv_navigation_action_internal.h"
#include "ios/web_view/internal/cwv_navigation_type_internal.h"
#include "ios/web_view/internal/cwv_web_view_internal.h"
#include "ios/web_view/internal/passwords/web_view_password_manager_client.h"
#include "ios/web_view/public/cwv_autofill_controller.h"

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
- (CWVAutofillController*)newAutofillController;
- (id<CRWResponderInputView>)webStateInputViewProvider:(web::WebState*)webState;
@end

@interface BraveWebView ()
@property(nonatomic, weak)
    id<AIChatUIHandlerBridge, AIChatAssociatedContentPageFetcher>
        aiChatUIHandler;
@property(nonatomic, weak) id<WalletPageHandlerBridge> walletPageHandler;
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
  ai_chat::UIHandlerBridgeHolder::GetOrCreateForWebState(self.webState)
      ->SetBridge(self.aiChatUIHandler);
  ai_chat::AIChatTabHelper::GetOrCreateForWebState(self.webState)
      ->SetPageFetcher(self.aiChatUIHandler);
  brave_wallet::PageHandlerBridgeHolder::GetOrCreateForWebState(self.webState)
      ->SetBridge(self.walletPageHandler);

  ProfileIOS* profile =
      ProfileIOS::FromBrowserState(self.webState->GetBrowserState());
  ai_chat::TabTrackerService* tab_tracker_service =
      ai_chat::TabTrackerServiceFactory::GetForProfile(profile);
  if (tab_tracker_service) {
    ai_chat::TabDataWebStateObserver::CreateForWebState(self.webState,
                                                        *tab_tracker_service);
  }
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

- (CWVAutofillController*)newAutofillController {
  // Reimplements CWVWebView's `newAutofillController` method to  create a
  // CWVAutofillController using Chrome factories instead of `//ios/web_view`
  // specific factories.
  if (!base::FeatureList::IsEnabled(
          brave::features::kUseChromiumWebViewsAutofill)) {
    return nil;
  }
  if (!self.webState) {
    return nil;
  }
  ProfileIOS* profile =
      ProfileIOS::FromBrowserState(self.webState->GetBrowserState());
  AutofillAgent* autofillAgent =
      [[AutofillAgent alloc] initWithPrefService:profile->GetPrefs()
                                        webState:self.webState];

  auto profile_store = IOSChromeProfilePasswordStoreFactory::GetForProfile(
      profile, ServiceAccessType::EXPLICIT_ACCESS);
  auto account_store = IOSChromeAccountPasswordStoreFactory::GetForProfile(
      profile, ServiceAccessType::EXPLICIT_ACCESS);
  auto passwordManagerClient =
      std::make_unique<BraveWebViewPasswordManagerClient>(
          self.webState, SyncServiceFactory::GetForProfile(profile),
          profile->GetPrefs(), IdentityManagerFactory::GetForProfile(profile),
          autofill::AutofillLogRouterFactory::GetForProfile(profile),
          profile_store.get(), account_store.get(),
          /* reuse_manager */ nullptr,
          /* requirements_service */ nullptr);

  auto passwordManager = std::make_unique<password_manager::PasswordManager>(
      passwordManagerClient.get());

  PasswordFormHelper* formHelper =
      [[PasswordFormHelper alloc] initWithWebState:self.webState];
  PasswordSuggestionHelper* suggestionHelper =
      [[PasswordSuggestionHelper alloc] initWithWebState:self.webState
                                         passwordManager:passwordManager.get()];
  PasswordControllerDriverHelper* driverHelper =
      [[PasswordControllerDriverHelper alloc] initWithWebState:self.webState];
  SharedPasswordController* passwordController =
      [[SharedPasswordController alloc] initWithWebState:self.webState
                                                 manager:passwordManager.get()
                                              formHelper:formHelper
                                        suggestionHelper:suggestionHelper
                                            driverHelper:driverHelper];
  return [[CWVAutofillController alloc]
           initWithWebState:self.webState
       createAutofillClient:
           base::BindRepeating(&autofill::BraveWebViewAutofillClientIOS::Create)
              autofillAgent:autofillAgent
            passwordManager:std::move(passwordManager)
      passwordManagerClient:std::move(passwordManagerClient)
         passwordController:passwordController];
}

- (CWVTranslationController*)translationController {
  NOTREACHED();
  return nil;
}

#pragma mark - CRWWebStateDelegate

- (id<CRWResponderInputView>)webStateInputViewProvider:
    (web::WebState*)webState {
  if (self.inputAccessoryViewController != nil ||
      self.inputViewController != nil || self.inputView != nil ||
      self.inputAccessoryView != nil) {
    return self;
  }
  return nil;
}

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

@implementation BraveWebView (AIChat)

- (void)setAiChatUIHandler:
    (id<AIChatUIHandlerBridge, AIChatAssociatedContentPageFetcher>)bridge {
  _aiChatUIHandler = bridge;
  ai_chat::UIHandlerBridgeHolder::GetOrCreateForWebState(self.webState)
      ->SetBridge(bridge);
  ai_chat::AIChatTabHelper::GetOrCreateForWebState(self.webState)
      ->SetPageFetcher(self.aiChatUIHandler);
}

@end

@implementation BraveWebView (Wallet)

- (void)setWalletPageHandler:(id<WalletPageHandlerBridge>)bridge {
  _walletPageHandler = bridge;
  brave_wallet::PageHandlerBridgeHolder::GetOrCreateForWebState(self.webState)
      ->SetBridge(bridge);
}

@end
