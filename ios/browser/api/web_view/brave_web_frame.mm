// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/web_view/brave_web_frame.h"

#include "base/strings/sys_string_conversions.h"
#import "brave/ios/browser/api/url/url_origin_ios+private.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

@implementation BraveWebFrame
- (instancetype)initWithWebFrame:(web::WebFrame*)webFrame {
  if ((self = [super init])) {
    _frameID = base::SysUTF8ToNSString(webFrame->GetFrameId());
    _mainFrame = webFrame->IsMainFrame();
    _securityOrigin =
        [[URLOriginIOS alloc] initWithOrigin:webFrame->GetSecurityOrigin()];
    _url = net::NSURLWithGURL(webFrame->GetUrl());
  }
  return self;
}
@end
