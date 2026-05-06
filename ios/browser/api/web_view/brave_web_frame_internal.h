// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_FRAME_INTERNAL_H_
#define BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_FRAME_INTERNAL_H_

#import <Foundation/Foundation.h>

#include "brave/ios/browser/api/web_view/brave_web_frame.h"

namespace web {

class WebFrame;

}  // namespace web

NS_ASSUME_NONNULL_BEGIN

@interface BraveWebFrame (Private)

- (instancetype)initWithWebFrame:(web::WebFrame*)webFrame;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_FRAME_INTERNAL_H_
