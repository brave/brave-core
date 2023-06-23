/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/sync/brave_sync_internals.h"

#include "base/memory/weak_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/components/webui/web_ui_url_constants.h"
#import "ios/web/js_messaging/web_view_web_state_map.h"
#import "ios/web/public/browser_state.h"
#import "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/thread/web_thread.h"
#import "ios/web/public/web_state.h"
#import "ios/web/public/web_state_delegate_bridge.h"
#import "ios/web/public/web_state_observer_bridge.h"
#import "ios/web/public/webui/web_ui_ios.h"
#import "ios/web/web_state/ui/crw_web_controller.h"
#import "ios/web/web_state/ui/wk_web_view_configuration_provider.h"
#import "ios/web/web_state/web_state_impl.h"
#import "ios/web/webui/web_ui_ios_impl.h"
#import "net/base/mac/url_conversions.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

WKWebView* EnsureWebViewCreatedWithConfiguration(
    WebState* web_state,
    WKWebViewConfiguration* configuration) {
  WebStateImpl* impl = static_cast<WebStateImpl*>(web_state);
  BrowserState* browser_state = impl->GetBrowserState();
  CRWWebController* web_controller = impl->GetWebController();

  WKWebViewConfigurationProvider& provider =
      WKWebViewConfigurationProvider::FromBrowserState(browser_state);
  provider.ResetWithWebViewConfiguration(configuration);

  // |web_controller| will get the |configuration| from the |provider| to create
  // the webView to return.
  return [web_controller ensureWebViewCreated];
}

}  // namespace web

@protocol CRWebViewPrivate <NSObject>
- (void)setWebView:(WKWebView*)webView;
@end

@interface BraveSyncInternalsController () <CRWWebStateDelegate,
                                            CRWWebStateObserver> {
  ChromeBrowserState* _chromeBrowserState;
  std::unique_ptr<web::WebState> _webState;
  std::unique_ptr<web::WebStateObserverBridge> _webStateObserver;
  std::unique_ptr<web::WebStateDelegateBridge> _webStateDelegate;

  WKWebView* _webView;
  CRWWebController* _webController;
}
@end

@implementation BraveSyncInternalsController
- (instancetype)initWithBrowserState:(ChromeBrowserState*)mainBrowserState {
  if ((self = [super init])) {
    _chromeBrowserState = mainBrowserState;
    web::WebState::CreateParams webStateCreateParams(mainBrowserState);
    _webState = web::WebState::Create(webStateCreateParams);

    _webStateObserver = std::make_unique<web::WebStateObserverBridge>(self);
    _webState->AddObserver(_webStateObserver.get());

    _webStateDelegate = std::make_unique<web::WebStateDelegateBridge>(self);
    _webState->SetDelegate(_webStateDelegate.get());

    _webController =
        static_cast<web::WebStateImpl*>(_webState.get())->GetWebController();
    _webView = [_webController ensureWebViewCreated];
  }
  return self;
}

- (void)dealloc {
  [[_webController view] removeFromSuperview];
  _webController = nullptr;
  _webView = nullptr;
  _webState->RemoveObserver(_webStateObserver.get());
  _webStateObserver.reset();
  _webState.reset();
  _chromeBrowserState = nullptr;
}

- (void)loadView {
  self.view = [_webController view];
}

- (void)viewDidLoad {
  [super viewDidLoad];

  [self loadURL:[NSString stringWithFormat:@"%s://%s", kChromeUIScheme,
                                           kChromeUISyncInternalsHost]];
}

- (void)loadURL:(NSString*)urlString {
  GURL url = net::GURLWithNSURL([NSURL URLWithString:urlString]);
  web::NavigationManager::WebLoadParams params(url);
  params.transition_type = ui::PAGE_TRANSITION_TYPED;
  _webState->GetNavigationManager()->LoadURLWithParams(params);
}

/**
    EVERYTHING below this line is NOT exported or used yet.
    It's too beta and requires a lot of refactoring of iOS code.
    Once we have a better way to integrate external WKWebView's, we can
   uncomment it and test it out. Until then, it's best we don't use it.
        - Brandon T.
*/

//- (UIView*)getViewForDisplay {
//  return [_webController view];
//}
//
//- (WKWebView*)getWebViewForDisplay {
//  return _webView;
//}
//
//- (void)setCustomWebView:(WKWebView*)webView {
//  if ([_webController
//  respondsToSelector:NSSelectorFromString(@"setWebView:")]) {
//    [((id<CRWebViewPrivate>)_webController) setWebView:webView];
//  }
//}

//- (void)setCustomWebView:(WKWebView*)webView {
//  web::WebState::CreateParams webStateCreateParams(_chromeBrowserState);
//  std::unique_ptr<web::WebState> webState =
//  web::WebState::Create(webStateCreateParams);
//
//  std::unique_ptr<web::WebStateObserverBridge> webStateObserver =
//  std::make_unique<web::WebStateObserverBridge>(self);
//  webState->AddObserver(_webStateObserver.get());
//
//  std::unique_ptr<web::WebStateDelegateBridge> webStateDelegate =
//  std::make_unique<web::WebStateDelegateBridge>(self);
//  webState->SetDelegate(webStateDelegate.get());
//
//  web::WKWebViewConfigurationProvider& config_provider =
//        web::WKWebViewConfigurationProvider::FromBrowserState(_chromeBrowserState);
//
//  web::WebStateImpl* webStateImpl =
//  static_cast<web::WebStateImpl*>(webState.get());
//  web::WebViewWebStateMap::FromBrowserState(
//      webStateImpl->GetBrowserState())
//      ->SetAssociatedWebViewForWebState(webView, webStateImpl);
//
//  if (webView) {
//    webStateImpl->RemoveAllWebFrames();
//    [webView stopLoading];
//
//    config_provider.ResetWithWebViewConfiguration(webView.configuration);
//
//    webStateImpl->ClearWebUI();
//    webStateImpl->CreateWebUI(GURL("chrome://sync-internals"));
//
//    GURL url = net::GURLWithNSURL([NSURL
//    URLWithString:@"chrome://sync-internals"]);
//    web::NavigationManager::WebLoadParams params(url);
//    params.transition_type = ui::PAGE_TRANSITION_TYPED;
//    _webState->GetNavigationManager()->LoadURLWithParams(params);
//
//    [webView setWebState:webState.release()];
//    [webView setWebStateObserver:webStateObserver.release()];
//    [webView setWebStateDelegate:webStateDelegate.release()];
//  }
//}

// MARK: - WebStateObserver implementation.

- (void)webState:(web::WebState*)webState
    didStartNavigation:(web::NavigationContext*)navigation {
}

- (void)webState:(web::WebState*)webState
    didFinishNavigation:(web::NavigationContext*)navigation {
}

- (void)webState:(web::WebState*)webState didLoadPageWithSuccess:(BOOL)success {
  DCHECK_EQ(_webState.get(), webState);
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
  NOTREACHED();
}
@end
