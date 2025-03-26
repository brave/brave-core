/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/web_view/public/cwv_web_view_extras.h"

#include "base/apple/foundation_util.h"
#include "base/json/json_writer.h"
#include "base/strings/sys_string_conversions.h"
#include "ios/web/common/user_agent.h"
#include "ios/web/js_messaging/java_script_feature_manager.h"
#include "ios/web/js_messaging/web_frame_internal.h"
#include "ios/web/js_messaging/web_view_js_utils.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/web_state/ui/crw_web_controller.h"
#include "ios/web/web_state/ui/wk_web_view_configuration_provider.h"
#include "ios/web/web_state/web_state_impl.h"
#include "ios/web_view/internal/cwv_web_view_configuration_internal.h"
#include "ios/web_view/internal/cwv_web_view_internal.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#include "ios/web_view/public/cwv_navigation_delegate.h"
#include "net/base/apple/url_conversions.h"

const CWVUserAgentType CWVUserAgentTypeNone =
    static_cast<CWVUserAgentType>(web::UserAgentType::NONE);
const CWVUserAgentType CWVUserAgentTypeAutomatic =
    static_cast<CWVUserAgentType>(web::UserAgentType::AUTOMATIC);
const CWVUserAgentType CWVUserAgentTypeMobile =
    static_cast<CWVUserAgentType>(web::UserAgentType::MOBILE);
const CWVUserAgentType CWVUserAgentTypeDesktop =
    static_cast<CWVUserAgentType>(web::UserAgentType::DESKTOP);

@implementation CWVWebView (Extras)

+ (BOOL)isRestoreDataValid:(NSData*)data {
  return [self _isRestoreDataValid:data];
}

- (void)updateScripts {
  // This runs `UpdateScripts` on the configuration provider which we will need
  // to call in-place of `-[WKUserContentController removeAllUserScripts]` until
  // all Brave JavaScript features are ported over to actual Chromium
  // JavascriptFeature types and added to BraveWebClient
  web::WKWebViewConfigurationProvider& config_provider =
      web::WKWebViewConfigurationProvider::FromBrowserState(
          self.webState->GetBrowserState());
  config_provider.UpdateScripts();
}

- (void)createFullPagePDF:(void (^)(NSData* _Nullable))completionHandler {
  self.webState->CreateFullPagePdf(base::BindOnce(completionHandler));
}

- (BOOL)canTakeSnapshot {
  return self.webState->CanTakeSnapshot();
}

- (void)takeSnapshotWithRect:(CGRect)rect
           completionHandler:(void (^)(UIImage* _Nullable))completionHandler {
  self.webState->TakeSnapshot(rect, base::BindRepeating(completionHandler));
}

- (CWVUserAgentType)currentItemUserAgentType {
  return static_cast<CWVUserAgentType>(self.webState->GetNavigationManager()
                                           ->GetVisibleItem()
                                           ->GetUserAgentType());
}

- (void)reloadWithUserAgentType:(CWVUserAgentType)userAgentType {
  self.webState->GetNavigationManager()->ReloadWithUserAgentType(
      static_cast<web::UserAgentType>(userAgentType));
}

- (void)evaluateJavaScript:(NSString*)javaScriptString
              contentWorld:(WKContentWorld*)contentWorld
         completionHandler:(void (^)(id result, NSError* error))completion {
  web::WebFrame* mainFrame =
      self.webState->GetPageWorldWebFramesManager()->GetMainWebFrame();
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
            jsResult = [CWVWebView objectFromValue:result];
          }
          completion(jsResult, error);
        }
      }));
}

- (WKWebView*)internalWebView {
  CRWWebController* web_controller =
      web::WebStateImpl::FromWebState(self.webState)->GetWebController();
  return web_controller.webView;
}

- (WKWebViewConfiguration*)WKConfiguration {
  web::WKWebViewConfigurationProvider& config_provider =
      web::WKWebViewConfigurationProvider::FromBrowserState(
          self.webState->GetBrowserState());
  return config_provider.GetWebViewConfiguration();
}

- (NSURL*)originalRequestURLForLastCommitedNavigation {
  // Since CWVBackForwardItemListItem doesn't provide the original URL request
  web::NavigationItem* item =
      self.webState->GetNavigationManager()->GetLastCommittedItem();
  if (!item) {
    return nil;
  }
  return net::NSURLWithGURL(item->GetOriginalRequestURL());
}

- (NSString*)contentsMIMEType {
  return base::SysUTF8ToNSString(self.webState->GetContentsMimeType());
}

- (NSDate*)lastActiveTime {
  return self.webState->GetLastActiveTime().ToNSDate();
}

@end
