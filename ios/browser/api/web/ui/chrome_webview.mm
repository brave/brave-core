// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/web/ui/chrome_webview.h"

#include "base/memory/weak_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "components/profile_metrics/browser_profile_type.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state_manager.h"
#include "ios/components/webui/web_ui_url_constants.h"
#import "ios/web/js_messaging/web_view_web_state_map.h"
#import "ios/web/public/browser_state.h"
#import "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/thread/web_thread.h"
#include "ios/web/public/ui/java_script_dialog_presenter.h"
#import "ios/web/public/web_state.h"
#import "ios/web/public/web_state_delegate_bridge.h"
#import "ios/web/public/web_state_observer_bridge.h"
#import "ios/web/public/webui/web_ui_ios.h"
#import "ios/web/web_state/ui/crw_web_controller.h"
#import "ios/web/web_state/ui/wk_web_view_configuration_provider.h"
#import "ios/web/web_state/web_state_impl.h"
#import "ios/web/webui/web_ui_ios_impl.h"
#import "net/base/apple/url_conversions.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

@class ChromeWebViewController;

@protocol ChromeWebControllerUIDelegate <NSObject>
@optional
- (void)webController:(ChromeWebViewController*)controller
    runJavaScriptAlertPanelWithMessage:(NSString*)message
                               pageURL:(NSURL*)URL
                     completionHandler:(void (^)(void))completionHandler;

- (void)webController:(ChromeWebViewController*)controller
    runJavaScriptConfirmPanelWithMessage:(NSString*)message
                                 pageURL:(NSURL*)URL
                       completionHandler:(void (^)(BOOL))completionHandler;

- (void)webController:(ChromeWebViewController*)controller
    runJavaScriptTextInputPanelWithPrompt:(NSString*)prompt
                              defaultText:(NSString*)defaultText
                                  pageURL:(NSURL*)URL
                        completionHandler:
                            (void (^)(NSString* _Nullable))completionHandler;
@end

namespace {
class WebViewJavaScriptDialogPresenter final
    : public web::JavaScriptDialogPresenter {
 public:
  WebViewJavaScriptDialogPresenter(ChromeWebViewController* controller);

  WebViewJavaScriptDialogPresenter(const WebViewJavaScriptDialogPresenter&) =
      delete;
  WebViewJavaScriptDialogPresenter& operator=(
      const WebViewJavaScriptDialogPresenter&) = delete;

  ~WebViewJavaScriptDialogPresenter() override;

  void SetUIDelegate(id<ChromeWebControllerUIDelegate> ui_delegate);

  // web::JavaScriptDialogPresenter overrides:
  void RunJavaScriptAlertDialog(web::WebState* web_state,
                                const GURL& origin_url,
                                NSString* message_text,
                                base::OnceClosure callback) override;
  void RunJavaScriptConfirmDialog(
      web::WebState* web_state,
      const GURL& origin_url,
      NSString* message_text,
      base::OnceCallback<void(bool success)> callback) override;
  void RunJavaScriptPromptDialog(
      web::WebState* web_state,
      const GURL& origin_url,
      NSString* message_text,
      NSString* default_prompt_text,
      base::OnceCallback<void(NSString* user_input)> callback) override;
  void CancelDialogs(web::WebState* web_state) override;

 private:
  __weak ChromeWebViewController* controller_ = nil;
  __weak id<ChromeWebControllerUIDelegate> ui_delegate_ = nil;
};

WebViewJavaScriptDialogPresenter::WebViewJavaScriptDialogPresenter(
    ChromeWebViewController* controller)
    : controller_(controller), ui_delegate_(nullptr) {}

WebViewJavaScriptDialogPresenter::~WebViewJavaScriptDialogPresenter() = default;

void WebViewJavaScriptDialogPresenter::RunJavaScriptAlertDialog(
    web::WebState* web_state,
    const GURL& origin_url,
    NSString* message_text,
    base::OnceClosure callback) {
  SEL delegate_method = @selector(webController:
             runJavaScriptAlertPanelWithMessage:pageURL:completionHandler:);
  if (![ui_delegate_ respondsToSelector:delegate_method]) {
    std::move(callback).Run();
    return;
  }
  [ui_delegate_ webController:controller_
      runJavaScriptAlertPanelWithMessage:message_text
                                 pageURL:net::NSURLWithGURL(origin_url)
                       completionHandler:base::CallbackToBlock(
                                             std::move(callback))];
}

void WebViewJavaScriptDialogPresenter::RunJavaScriptConfirmDialog(
    web::WebState* web_state,
    const GURL& origin_url,
    NSString* message_text,
    base::OnceCallback<void(bool success)> callback) {
  SEL delegate_method = @selector(webController:
           runJavaScriptConfirmPanelWithMessage:pageURL:completionHandler:);
  if (![ui_delegate_ respondsToSelector:delegate_method]) {
    std::move(callback).Run(false);
    return;
  }
  [ui_delegate_ webController:controller_
      runJavaScriptConfirmPanelWithMessage:message_text
                                   pageURL:net::NSURLWithGURL(origin_url)
                         completionHandler:base::CallbackToBlock(
                                               std::move(callback))];
}

void WebViewJavaScriptDialogPresenter::RunJavaScriptPromptDialog(
    web::WebState* web_state,
    const GURL& origin_url,
    NSString* message_text,
    NSString* default_prompt_text,
    base::OnceCallback<void(NSString* user_input)> callback) {
  SEL delegate_method = @selector(webController:
          runJavaScriptTextInputPanelWithPrompt:defaultText:pageURL
                                               :completionHandler:);
  if (![ui_delegate_ respondsToSelector:delegate_method]) {
    std::move(callback).Run(nil);
    return;
  }
  [ui_delegate_ webController:controller_
      runJavaScriptTextInputPanelWithPrompt:message_text
                                defaultText:default_prompt_text
                                    pageURL:net::NSURLWithGURL(origin_url)
                          completionHandler:base::CallbackToBlock(
                                                std::move(callback))];
}

void WebViewJavaScriptDialogPresenter::CancelDialogs(web::WebState* web_state) {
}

void WebViewJavaScriptDialogPresenter::SetUIDelegate(
    id<ChromeWebControllerUIDelegate> ui_delegate) {
  ui_delegate_ = ui_delegate;
}
}  // namespace

@interface ChromeWebViewController () <CRWWebStateDelegate,
                                       CRWWebStateObserver,
                                       ChromeWebControllerUIDelegate> {
  raw_ptr<ChromeBrowserState> browser_state_;
  std::unique_ptr<web::WebState> web_state_;
  std::unique_ptr<web::WebStateObserverBridge> web_state_observer_;
  std::unique_ptr<web::WebStateDelegateBridge> web_state_delegate_;
  std::unique_ptr<WebViewJavaScriptDialogPresenter> dialog_presenter_;

  WKWebView* web_view_;
  CRWWebController* web_controller_;
}
@end

@implementation ChromeWebViewController
- (instancetype)initWithPrivateBrowsing:(bool)isPrivateBrowsing {
  if ((self = [super init])) {
    ChromeBrowserStateManager* browser_state_manager =
        GetApplicationContext()->GetChromeBrowserStateManager();

    browser_state_ =
        browser_state_manager->GetLastUsedBrowserStateDeprecatedDoNotUse()
            ->GetOriginalChromeBrowserState();

    if (isPrivateBrowsing) {
      browser_state_ = browser_state_->GetOffTheRecordChromeBrowserState();
    }

    web_state_ =
        web::WebState::Create(web::WebState::CreateParams(browser_state_));

    web_state_observer_ = std::make_unique<web::WebStateObserverBridge>(self);
    web_state_->AddObserver(web_state_observer_.get());

    web_state_delegate_ = std::make_unique<web::WebStateDelegateBridge>(self);
    web_state_->SetDelegate(web_state_delegate_.get());

    dialog_presenter_ =
        std::make_unique<WebViewJavaScriptDialogPresenter>(self);
    dialog_presenter_->SetUIDelegate(self);

    web_controller_ =
        static_cast<web::WebStateImpl*>(web_state_.get())->GetWebController();
    web_view_ = [web_controller_ ensureWebViewCreated];
  }
  return self;
}

- (void)dealloc {
  [[web_controller_ view] removeFromSuperview];
  web_controller_ = nullptr;
  web_view_ = nullptr;
  web_state_->RemoveObserver(web_state_observer_.get());
  web_state_observer_.reset();
  web_state_delegate_.reset();
  web_state_.reset();
  dialog_presenter_.reset();
  browser_state_ = nullptr;
}

- (WKWebView*)webView {
  return web_view_;
}

- (bool)isOffTheRecord {
  return profile_metrics::GetBrowserProfileType(browser_state_) ==
         profile_metrics::BrowserProfileType::kIncognito;
}

- (void)loadView {
  self.view = [web_controller_ view];
}

- (void)loadURL:(NSString*)urlString {
  GURL url = net::GURLWithNSURL([NSURL URLWithString:urlString]);
  web::NavigationManager::WebLoadParams params(url);
  params.transition_type = ui::PAGE_TRANSITION_TYPED;
  web_state_->GetNavigationManager()->LoadURLWithParams(params);
}

// MARK: - WebStateObserver implementation.

- (void)webState:(web::WebState*)webState
    didStartNavigation:(web::NavigationContext*)navigation {
}

- (void)webState:(web::WebState*)webState
    didFinishNavigation:(web::NavigationContext*)navigation {
}

- (void)webState:(web::WebState*)webState didLoadPageWithSuccess:(BOOL)success {
  DCHECK_EQ(web_state_.get(), webState);
}

// MARK: - WebStateDelegate implementation.

- (void)webState:(web::WebState*)webView
    contextMenuConfigurationForParams:(const web::ContextMenuParams&)params
                    completionHandler:(void (^)(UIContextMenuConfiguration*))
                                          completionHandler {
  completionHandler(nil);
}

- (void)webStateDestroyed:(web::WebState*)webState {
  // The WebState is owned by the current instance, and the observer bridge
  // is unregistered before the WebState is destroyed, so this event should
  // never happen.
  NOTREACHED_IN_MIGRATION();
}

// MARK: - CRWWebStateDelegate implementation

- (web::JavaScriptDialogPresenter*)javaScriptDialogPresenterForWebState:
    (web::WebState*)webState {
  return dialog_presenter_.get();
}

// MARK: - ChromeWebControllerUIDelegate implementation

- (void)webController:(ChromeWebViewController*)controller
    runJavaScriptAlertPanelWithMessage:(NSString*)message
                               pageURL:(NSURL*)URL
                     completionHandler:(void (^)(void))handler {
  UIAlertController* alert =
      [UIAlertController alertControllerWithTitle:nil
                                          message:message
                                   preferredStyle:UIAlertControllerStyleAlert];

  [alert addAction:[UIAlertAction actionWithTitle:@"Ok"
                                            style:UIAlertActionStyleDefault
                                          handler:^(UIAlertAction* action) {
                                            handler();
                                          }]];

  [controller presentViewController:alert animated:YES completion:nil];
}

- (void)webController:(ChromeWebViewController*)controller
    runJavaScriptConfirmPanelWithMessage:(NSString*)message
                                 pageURL:(NSURL*)URL
                       completionHandler:(void (^)(BOOL))handler {
  UIAlertController* alert =
      [UIAlertController alertControllerWithTitle:nil
                                          message:message
                                   preferredStyle:UIAlertControllerStyleAlert];

  [alert addAction:[UIAlertAction actionWithTitle:@"Ok"
                                            style:UIAlertActionStyleDefault
                                          handler:^(UIAlertAction* action) {
                                            handler(YES);
                                          }]];
  [alert addAction:[UIAlertAction actionWithTitle:@"Cancel"
                                            style:UIAlertActionStyleCancel
                                          handler:^(UIAlertAction* action) {
                                            handler(NO);
                                          }]];

  [controller presentViewController:alert animated:YES completion:nil];
}

- (void)webController:(ChromeWebViewController*)controller
    runJavaScriptTextInputPanelWithPrompt:(NSString*)prompt
                              defaultText:(NSString*)defaultText
                                  pageURL:(NSURL*)URL
                        completionHandler:(void (^)(NSString*))handler {
  UIAlertController* alert =
      [UIAlertController alertControllerWithTitle:nil
                                          message:prompt
                                   preferredStyle:UIAlertControllerStyleAlert];

  [alert addTextFieldWithConfigurationHandler:^(UITextField* textField) {
    textField.text = defaultText;
  }];

  __weak UIAlertController* weakAlert = alert;
  [alert addAction:[UIAlertAction
                       actionWithTitle:@"Ok"
                                 style:UIAlertActionStyleDefault
                               handler:^(UIAlertAction* action) {
                                 NSString* textInput =
                                     weakAlert.textFields.firstObject.text;
                                 handler(textInput);
                               }]];
  [alert addAction:[UIAlertAction actionWithTitle:@"Cancel"
                                            style:UIAlertActionStyleCancel
                                          handler:^(UIAlertAction* action) {
                                            handler(nil);
                                          }]];

  [controller presentViewController:alert animated:YES completion:nil];
}
@end
