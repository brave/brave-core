/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/p3a/brave_histograms_controller.h"

#include "base/strings/sys_string_conversions.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/url/chrome_url_constants.h"
#include "ios/components/webui/web_ui_url_constants.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/web_state_delegate_bridge.h"
#include "ios/web/public/web_state_observer_bridge.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/web_state/ui/crw_web_controller.h"
#include "ios/web/web_state/ui/wk_web_view_configuration_provider.h"
#include "ios/web/web_state/web_state_impl.h"
#include "ios/web/webui/web_ui_ios_impl.h"
#include "net/base/mac/url_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface BraveHistogramsController () <CRWWebStateDelegate,
                                         CRWWebStateObserver> {
  std::unique_ptr<web::WebState> _webState;
  std::unique_ptr<web::WebStateObserverBridge> _webStateObserver;
  std::unique_ptr<web::WebStateDelegateBridge> _webStateDelegate;

  WKWebView* _webView;
  CRWWebController* _webController;
}
@end

@implementation BraveHistogramsController

- (instancetype)initWithBrowserState:(ChromeBrowserState*)mainBrowserState {
  if ((self = [super init])) {
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
}

- (void)loadView {
  self.view = [_webController view];
}

- (void)viewDidLoad {
  [super viewDidLoad];

  [self loadURL:[NSString stringWithFormat:@"%s://%s", kChromeUIScheme,
                                           kChromeUIHistogramHost]];
}

- (void)loadURL:(NSString*)urlString {
  GURL url = net::GURLWithNSURL([NSURL URLWithString:urlString]);
  web::NavigationManager::WebLoadParams params(url);
  params.transition_type = ui::PAGE_TRANSITION_TYPED;
  _webState->GetNavigationManager()->LoadURLWithParams(params);
}

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
