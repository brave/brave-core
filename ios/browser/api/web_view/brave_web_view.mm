// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/web_view/brave_web_view.h"

#include <Foundation/Foundation.h>

#include <memory>

#include "base/apple/foundation_util.h"
#include "base/notreached.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/ios/browser/ai_chat_associated_content_page_fetcher.h"
#include "brave/components/ai_chat/ios/browser/ai_chat_tab_helper.h"
#include "brave/components/brave_talk/buildflags/buildflags.h"
#include "brave/ios/browser/ai_chat/ai_chat_distiller_javascript_feature.h"
#include "brave/ios/browser/ai_chat/ai_chat_ui_handler_bridge_holder.h"
#include "brave/ios/browser/ai_chat/tab_data_web_state_observer.h"
#include "brave/ios/browser/ai_chat/tab_tracker_service_factory.h"
#include "brave/ios/browser/api/web_view/autofill/brave_web_view_autofill_client.h"
#include "brave/ios/browser/api/web_view/passwords/brave_web_view_password_manager_client.h"
#include "brave/ios/browser/brave_ads/ads_tab_helper.h"
#include "brave/ios/browser/brave_talk/brave_talk_tab_helper_bridge.h"
#include "brave/ios/browser/favicon/brave_ios_web_favicon_driver.h"
#include "brave/ios/browser/ui/web_view/features.h"
#include "brave/ios/browser/ui/webui/brave_wallet/wallet_page_handler_bridge_holder.h"
#include "brave/ios/browser/web/document_fetch/document_fetch_javascript_feature.h"
#include "brave/ios/browser/web/force_paste/force_paste_javascript_feature.h"
#include "brave/ios/browser/web/logins/logins_tab_helper.h"
#include "brave/ios/browser/web/logins/logins_tab_helper_bridge.h"
#include "brave/ios/browser/web/page_metadata/page_metadata_javascript_feature.h"
#include "brave/ios/browser/web/reader_mode/reader_mode_javascript_feature.h"
#include "components/autofill/core/browser/logging/log_manager.h"
#include "components/autofill/core/browser/logging/log_router.h"
#include "components/autofill/ios/browser/autofill_agent.h"
#include "components/autofill/ios/browser/autofill_client_ios.h"
#include "components/favicon/core/favicon_driver_observer.h"
#include "components/favicon/core/favicon_service.h"
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
#include "ios/chrome/browser/favicon/model/favicon_service_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_account_password_store_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_profile_password_store_factory.h"
#include "ios/chrome/browser/passwords/model/password_controller.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/signin/model/identity_manager_factory.h"
#include "ios/chrome/browser/sync/model/sync_service_factory.h"
#include "ios/chrome/browser/tabs/model/tab_helper_util.h"
#include "ios/chrome/browser/web/model/print/print_handler.h"
#include "ios/chrome/browser/web/model/print/print_tab_helper.h"
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
#include "net/base/apple/url_conversions.h"
#include "net/http/http_status_code.h"

#if BUILDFLAG(ENABLE_BRAVE_TALK)
#include "brave/ios/browser/brave_talk/brave_talk_tab_helper.h"
#endif

@interface BraveNavigationAction ()
- (instancetype)initWithRequest:(NSURLRequest*)request
                    requestInfo:
                        (web::WebStatePolicyDecider::RequestInfo)requestInfo;
@end

@protocol FaviconDriverObserverBridge <NSObject>
@required
- (void)faviconDriverDidUpdateFavicon:(favicon::FaviconDriver*)driver;
@end

namespace {
ResetConfigurationCallback gDidResetConfigurationCallback;

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

  void ShouldAllowResponse(NSURLResponse* response,
                           ResponseInfo response_info,
                           PolicyDecisionCallback callback) override {
    if (@available(iOS 26.2, *)) {
      // On iOS 26.2 and up, requests to pages that respond with a 204 or 205
      // status code will abort without calling any further delegate methods.
      // Due to a bug in Chromium's handling of the frame load interruption
      // error code (https://crbug.com/488310974) we have to handle these
      // responses in Brave as a workaround.
      if (auto http_response =
              base::apple::ObjCCast<NSHTTPURLResponse>(response)) {
        auto status_code = net::TryToGetHttpStatusCode(http_response.statusCode)
                               .value_or(net::HTTP_STATUS_CODE_MAX);
        if (status_code == net::HTTP_NO_CONTENT ||
            status_code == net::HTTP_RESET_CONTENT) {
          // Instead of allowing WebKit to abort this navigation, we'll just
          // respond with a cancellation so that the navigation doesnt fail
          // automatically with a frame load interruption error.
          std::move(callback).Run(
              web::WebStatePolicyDecider::PolicyDecision::Cancel());
          return;
        }
      }
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

class FaviconDriverObserver : public favicon::FaviconDriverObserver {
 public:
  explicit FaviconDriverObserver(id<FaviconDriverObserverBridge> bridge)
      : bridge_(bridge) {}

  // favicon::FaviconDriverObserver
  void OnFaviconUpdated(favicon::FaviconDriver* favicon_driver,
                        NotificationIconType notification_icon_type,
                        const GURL& icon_url,
                        bool icon_url_changed,
                        const gfx::Image& image) override {
    if (bridge_) {
      [bridge_ faviconDriverDidUpdateFavicon:favicon_driver];
    }
  }

 private:
  __weak id<FaviconDriverObserverBridge> bridge_;
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

@interface BraveWebView () <FaviconDriverObserverBridge>
@property(nonatomic, weak)
    id<AIChatUIHandlerBridge, AIChatAssociatedContentPageFetcher>
        aiChatUIHandler;
@property(nonatomic, weak) id<WalletPageHandlerBridge> walletPageHandler;
@property(nonatomic, weak) id<LoginsTabHelperBridge> loginsHelper;
#if BUILDFLAG(ENABLE_BRAVE_TALK)
@property(nonatomic, weak) id<BraveTalkTabHelperBridge> braveTalkHelper;
#endif
@property(nonatomic, weak) id<PrintHandler> printHandler;
@end

@implementation BraveWebView {
  std::unique_ptr<BraveWebViewWebStatePolicyDecider> _webStatePolicyDecider;
  std::unique_ptr<FaviconDriverObserver> _faviconObserver;
}

// These are shadowed CWVWebView properties
@dynamic navigationDelegate, UIDelegate;

- (void)dealloc {
  if (self.webState) {
    BraveWebViewHolder::RemoveFromWebState(self.webState);
  }
  if (auto* faviconDriver =
          brave_favicon::BraveIOSWebFaviconDriver::FromWebState(
              self.webState)) {
    faviconDriver->RemoveObserver(self.faviconDriverObserver);
  }
}

- (FaviconDriverObserver*)faviconDriverObserver {
  if (!_faviconObserver) {
    _faviconObserver = std::make_unique<FaviconDriverObserver>(self);
  }
  return _faviconObserver.get();
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

+ (ResetConfigurationCallback)didResetConfiguration {
  return gDidResetConfigurationCallback;
}

+ (void)setDidResetConfiguration:(ResetConfigurationCallback)callback {
  gDidResetConfigurationCallback = callback;
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

  if (auto* favicon_driver =
          brave_favicon::BraveIOSWebFaviconDriver::FromWebState(
              self.webState)) {
    // This matches the values set on FaviconDriver from the Swift side
    favicon_driver->SetMaximumFaviconImageSize(/*max_image_width=*/1024,
                                               /*max_image_height=*/1024);
    favicon_driver->AddObserver(self.faviconDriverObserver);
  }
}

- (void)attachSecurityInterstitialHelpersToWebStateIfNecessary {
  [super attachSecurityInterstitialHelpersToWebStateIfNecessary];
  AttachTabHelpers(self.webState);

  if (ai_chat::features::IsAIChatWebUIEnabled()) {
    ai_chat::UIHandlerBridgeHolder::CreateForWebState(self.webState);
    ai_chat::UIHandlerBridgeHolder::FromWebState(self.webState)
        ->SetBridge(self.aiChatUIHandler);
    ai_chat::AIChatTabHelper::CreateForWebState(self.webState);
    ai_chat::AIChatTabHelper::FromWebState(self.webState)
        ->SetPageFetcher(self.aiChatUIHandler);
  }
  brave_wallet::PageHandlerBridgeHolder::CreateForWebState(self.webState);
  brave_wallet::PageHandlerBridgeHolder::FromWebState(self.webState)
      ->SetBridge(self.walletPageHandler);

  ProfileIOS* profile =
      ProfileIOS::FromBrowserState(self.webState->GetBrowserState());
  ai_chat::TabTrackerService* tab_tracker_service =
      ai_chat::TabTrackerServiceFactory::GetForProfile(profile);
  if (tab_tracker_service) {
    ai_chat::TabDataWebStateObserver::CreateForWebState(self.webState,
                                                        *tab_tracker_service);
  }

  brave_ads::AdsTabHelper::MaybeCreateForWebState(self.webState);
#if BUILDFLAG(ENABLE_BRAVE_TALK)
  BraveTalkTabHelper::CreateForWebState(self.webState);
  BraveTalkTabHelper::FromWebState(self.webState)
      ->SetBridge(self.braveTalkHelper);
#endif

  LoginsTabHelper::MaybeCreateForWebState(self.webState, _loginsHelper);

  if (base::FeatureList::IsEnabled(
          brave::features::kUseProfileWebViewConfiguration)) {
    // When UseProfileWebViewConfiguration is removed, move this to
    // tab_helper_util.mm chromium_src override
    PrintTabHelper::CreateForWebState(self.webState);

    brave_favicon::BraveIOSWebFaviconDriver::CreateForWebState(
        self.webState,
        ios::FaviconServiceFactory::GetForProfile(
            profile->GetOriginalProfile(), ServiceAccessType::IMPLICIT_ACCESS));
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

#pragma mark - FaviconDriverObserverBridge

- (void)faviconDriverDidUpdateFavicon:(favicon::FaviconDriver*)driver {
  if ([self.UIDelegate respondsToSelector:@selector(webView:
                                              didUpdateFaviconStatus:)]) {
    [self.UIDelegate webView:self didUpdateFaviconStatus:self.faviconStatus];
  }
}

@end

@implementation BraveWebView (AdsNotifier)

- (void)notifyTabDidStartPlayingMedia {
  auto* adsTabHelper = brave_ads::AdsTabHelper::FromWebState(self.webState);
  if (!adsTabHelper) {
    return;
  }
  adsTabHelper->NotifyTabDidStartPlayingMedia();
}

- (void)notifyTabDidStopPlayingMedia {
  auto* adsTabHelper = brave_ads::AdsTabHelper::FromWebState(self.webState);
  if (!adsTabHelper) {
    return;
  }
  adsTabHelper->NotifyTabDidStopPlayingMedia();
}

@end

@implementation BraveWebView (AIChatDistiller)

- (void)fetchMainArticle:(void (^)(NSString* text))completionHandler {
  AIChatDistillerJavaScriptFeature::GetInstance()->GetMainArticle(
      self.webState, base::BindOnce(^(std::string text) {
        completionHandler(base::SysUTF8ToNSString(text));
      }));
}

@end

@implementation BraveWebView (AIChat)

- (void)setAiChatUIHandler:
    (id<AIChatUIHandlerBridge, AIChatAssociatedContentPageFetcher>)bridge {
  _aiChatUIHandler = bridge;
  if (ai_chat::features::IsAIChatWebUIEnabled()) {
    ai_chat::UIHandlerBridgeHolder::CreateForWebState(self.webState);
    ai_chat::UIHandlerBridgeHolder::FromWebState(self.webState)
        ->SetBridge(bridge);
    ai_chat::AIChatTabHelper::CreateForWebState(self.webState);
    ai_chat::AIChatTabHelper::FromWebState(self.webState)
        ->SetPageFetcher(self.aiChatUIHandler);
  }
}

@end

@implementation BraveWebView (Wallet)

- (void)setWalletPageHandler:(id<WalletPageHandlerBridge>)bridge {
  _walletPageHandler = bridge;
  brave_wallet::PageHandlerBridgeHolder::CreateForWebState(self.webState);
  brave_wallet::PageHandlerBridgeHolder::FromWebState(self.webState)
      ->SetBridge(bridge);
}

@end

@implementation BraveWebView (ForcePaste)

- (void)forcePasteContents:(NSString*)contents {
  ForcePasteJavaScriptFeature::GetInstance()->ForcePaste(
      self.webState, base::SysNSStringToUTF8(contents));
}

@end

@implementation BraveWebView (PageMetadata)

- (void)fetchMetadata:(void (^)(NSString* json))completionHandler {
  PageMetadataJavaScriptFeature::GetInstance()->GetMetadata(
      self.webState, base::BindOnce(^(const base::Value* value) {
        if (value && value->is_string()) {
          completionHandler(base::SysUTF8ToNSString(value->GetString()));
          return;
        }
        completionHandler(nil);
      }));
}

@end

@implementation BraveWebView (Logins)

- (void)setLoginsHelper:(id<LoginsTabHelperBridge>)loginsHelper {
  _loginsHelper = loginsHelper;
  auto* tab_helper = LoginsTabHelper::FromWebState(self.webState);
  if (tab_helper) {
    tab_helper->SetBridge(loginsHelper);
  }
}

@end

@implementation BraveWebView (DocumentFetch)

- (void)downloadDocumentAtURL:(NSURL*)url
            completionHandler:
                (void (^)(NSInteger statusCode,
                          NSData* _Nullable data))completionHandler {
  DocumentFetchJavaScriptFeature::GetInstance()->DownloadDocument(
      self.webState, net::GURLWithNSURL(url),
      base::BindOnce(^(int statusCode, const std::string& base64Data) {
        NSData* data = nil;
        if (!base64Data.empty()) {
          data = [[NSData alloc]
              initWithBase64EncodedString:base::SysUTF8ToNSString(base64Data)
                                  options:0];
        }
        completionHandler(statusCode, data);
      }));
}

@end

@implementation BraveWebView (ReaderMode)

- (void)checkReadability:(void (^)(NSString* _Nullable json))completionHandler {
  brave::ReaderModeJavaScriptFeature::GetInstance()->CheckReadability(
      self.webState, base::BindOnce(^(const std::string& json) {
        completionHandler(json.empty() ? nil : base::SysUTF8ToNSString(json));
      }));
}

- (void)setReaderModeTheme:(NSString*)theme
                  fontType:(NSString*)fontType
                  fontSize:(NSInteger)fontSize {
  base::DictValue style;
  style.Set("theme", base::SysNSStringToUTF8(theme));
  style.Set("fontType", base::SysNSStringToUTF8(fontType));
  style.Set("fontSize", static_cast<int>(fontSize));
  brave::ReaderModeJavaScriptFeature::GetInstance()->SetStyle(self.webState,
                                                              style);
}

@end

@implementation BraveWebView (BraveTalk)

/// A bridge for handling Brave Talk tab features
- (void)setBraveTalkHelper:(id<BraveTalkTabHelperBridge>)braveTalkHelper {
#if BUILDFLAG(ENABLE_BRAVE_TALK)
  _braveTalkHelper = braveTalkHelper;
  BraveTalkTabHelper* tab_helper =
      BraveTalkTabHelper::FromWebState(self.webState);
  if (tab_helper) {
    tab_helper->SetBridge(braveTalkHelper);
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_TALK)
}

@end

@implementation BraveWebView (Print)

- (void)setPrintHandler:(id<PrintHandler>)printHandler {
  _printHandler = printHandler;
  if (PrintTabHelper* tab_helper =
          PrintTabHelper::FromWebState(self.webState)) {
    tab_helper->set_printer(printHandler);
  }
}

@end
