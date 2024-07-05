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

// Converts base::Value expected to be a dictionary or list to NSDictionary or
// NSArray, respectively.
id NSObjectFromCollectionValue(const base::Value* value) {
  DCHECK(value->is_dict() || value->is_list())
      << "Incorrect value type: " << value->type();

  std::string json;
  const bool success = base::JSONWriter::Write(*value, &json);
  DCHECK(success) << "Failed to convert base::Value to JSON";

  NSData* json_data = [NSData dataWithBytes:json.c_str() length:json.length()];
  id ns_object = [NSJSONSerialization JSONObjectWithData:json_data
                                                 options:kNilOptions
                                                   error:nil];
  DCHECK(ns_object) << "Failed to convert JSON to Collection";
  return ns_object;
}

// Converts base::Value to an appropriate Obj-C object.
// |value| must not be null.
id NSObjectFromValue(const base::Value* value) {
  switch (value->type()) {
    case base::Value::Type::NONE:
      return nil;
    case base::Value::Type::BOOLEAN:
      return @(value->GetBool());
    case base::Value::Type::INTEGER:
      return @(value->GetInt());
    case base::Value::Type::DOUBLE:
      return @(value->GetDouble());
    case base::Value::Type::STRING:
      return base::SysUTF8ToNSString(value->GetString());
    case base::Value::Type::BINARY:
      // Unsupported.
      return nil;
    case base::Value::Type::DICT:
    case base::Value::Type::LIST:
      return NSObjectFromCollectionValue(value);
  }
  return nil;
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

- (void)createPDF:(void (^)(NSData* _Nullable))completionHandler {
  self.webState->CreateFullPagePdf(base::BindOnce(completionHandler));
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
            jsResult = NSObjectFromValue(result);
          }
          completion(jsResult, error);
        }
      }));
}

- (WKWebView*)underlyingWebView {
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
  }
}

@end
