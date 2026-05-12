// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/common/url_scheme_util.h"
#include "ios/web/public/web_client.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"

#define IsAppSpecificURL(URL) \
  IsAppSpecificURL(URL) &&    \
      self.mojoFacade->IsWebUIMessageAllowedForFrame(origin_url, prompt)

#include <ios/web/web_state/ui/crw_wk_ui_handler.mm>

#undef IsAppSpecificURL

@implementation BraveCRWWKUIHandler

- (void)webView:(WKWebView*)webView
    runJavaScriptTextInputPanelWithPrompt:(NSString*)prompt
                              defaultText:(NSString*)defaultText
                         initiatedByFrame:(WKFrameInfo*)frame
                        completionHandler:
                            (void (^)(NSString* result))completionHandler {
  GURL origin_url(web::GURLOriginWithWKSecurityOrigin(frame.securityOrigin));
  // For standard web pages we'll see if the window.prompt needs to be handled
  // by a configured `JavaScriptFeature` to handle immediately without actually
  // displaying a JS prompt to the user
  if (web::UrlHasWebScheme(origin_url)) {
    NSString* result = nil;
    auto* promptHandler =
        web::JSPromptCommunicationHandler::GetJSPromptCommunicationHandler(
            self.webStateImpl);
    if (promptHandler && promptHandler->HandleJavaScriptPrompt(
                             net::GURLWithNSURL(frame.request.URL),
                             frame.isMainFrame, prompt, &result)) {
      completionHandler(result);
      return;
    }
  }
  [super webView:webView
      runJavaScriptTextInputPanelWithPrompt:prompt
                                defaultText:defaultText
                           initiatedByFrame:frame
                          completionHandler:completionHandler];
}
@end

#pragma clang diagnostic pop
