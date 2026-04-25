/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#include "base/memory/weak_ptr.h"
#import "brave/ios/browser/api/web/web_state/web_state.h"

#ifndef BRAVE_IOS_BROWSER_API_WEB_WEB_STATE_WEB_STATE_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_WEB_WEB_STATE_WEB_STATE_PRIVATE_H_

class Browser;

namespace web {
class WebState;
}  // namespace web

@interface WebState (Private)
- (instancetype)initWithBrowser:(Browser*)browser
                 isOffTheRecord:(bool)isOffTheRecord;
- (void)setTitle:(NSString*)title;
- (void)setURL:(NSURL*)url;
- (base::WeakPtr<web::WebState>)internalWebState;
@end

#endif  // BRAVE_IOS_BROWSER_API_WEB_WEB_STATE_WEB_STATE_PRIVATE_H_
