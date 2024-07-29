/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/web_view/public/cwv_web_view_extras.h"
#include "ios/chrome/browser/tabs/model/tab_helper_util.h"
#include "ios/web/common/user_agent.h"
#include "ios/web/js_messaging/java_script_feature_manager.h"
#include "ios/web/js_messaging/web_frame_internal.h"
#include "ios/web/js_messaging/web_view_js_utils.h"
#include "ios/web/web_state/ui/crw_web_controller.h"
#include "ios/web/web_state/ui/wk_web_view_configuration_provider.h"
#include "ios/web/web_state/web_state_impl.h"

// Comment here to stop formatter form moving it up
#include "src/ios/web_view/internal/cwv_web_view.mm"

@implementation CWVWebView (Extras)

- (void)updateScripts {
  // This runs `UpdateScripts` on the configuration provider which we will need
  // to call in-place of `-[WKUserContentController removeAllUserScripts]` until
  // all Brave JavaScript features are ported over to actual Chromium
  // JavascriptFeature types and added to BraveWebClient
  web::WKWebViewConfigurationProvider& config_provider =
      web::WKWebViewConfigurationProvider::FromBrowserState(
          _webState->GetBrowserState());
  config_provider.UpdateScripts();
}

- (void)createPDF:(void (^)(NSData* _Nullable))completionHandler {
  _webState->CreateFullPagePdf(base::BindOnce(completionHandler));
}

- (void)takeSnapshotWithRect:(CGRect)rect
           completionHandler:(void (^)(UIImage* _Nullable))completionHandler {
  _webState->TakeSnapshot(rect, base::BindRepeating(completionHandler));
}

- (CWVUserAgentType)currentItemUserAgentType {
  return static_cast<CWVUserAgentType>(
      _webState->GetNavigationManager()->GetVisibleItem()->GetUserAgentType());
}

- (void)reloadWithUserAgentType:(CWVUserAgentType)userAgentType {
  _webState->GetNavigationManager()->ReloadWithUserAgentType(
      static_cast<web::UserAgentType>(userAgentType));
}

- (void)evaluateJavaScript:(NSString*)javaScriptString
              contentWorld:(WKContentWorld*)contentWorld
         completionHandler:(void (^)(id result, NSError* error))completion {
  web::WebFrame* mainFrame =
      _webState->GetPageWorldWebFramesManager()->GetMainWebFrame();
  if (!mainFrame) {
    if (completion) {
      completion(nil, [NSError errorWithDomain:@"org.chromium.chromewebview"
                                          code:0
                                      userInfo:nil]);
    }
    return;
  }
  web::WebFrameInternal* webFrame = mainFrame->GetWebFrameInternal();
  auto worlds = web::JavaScriptFeatureManager::FromBrowserState(
                    self.configuration.browserState)
                    ->GetAllContentWorlds();

  auto jsContentWorld =
      std::find_if(worlds.begin(), worlds.end(), [&contentWorld](auto world) {
        return world->GetWKContentWorld() == contentWorld;
      });
  if (jsContentWorld == worlds.end()) {
    if (completion) {
      completion(nil, [NSError errorWithDomain:@"org.chromium.chromewebview"
                                          code:0
                                      userInfo:nil]);
    }
    return;
  }

  webFrame->ExecuteJavaScriptInContentWorld(
      base::SysNSStringToUTF16(javaScriptString), *jsContentWorld,
      base::BindOnce(^(const base::Value* result, NSError* error) {
        if (completion) {
          id jsResult = nil;
          if (!error && result) {
            jsResult = NSObjectFromValue(result);
          }
          completion(jsResult, error);
        }
      }));
}

- (WKWebView*)underlyingWebView {
  CRWWebController* web_controller =
      web::WebStateImpl::FromWebState(_webState.get())->GetWebController();
  return [web_controller ensureWebViewCreated];
}

- (WKWebViewConfiguration*)WKConfiguration {
  web::WKWebViewConfigurationProvider& config_provider =
      web::WKWebViewConfigurationProvider::FromBrowserState(
          _webState->GetBrowserState());
  return config_provider.GetWebViewConfiguration();
}

@end
