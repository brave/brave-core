/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/web/web_state/web_state.h"
#include "brave/ios/browser/api/web/web_state/web_state_native.h"

#include <memory>

#include "base/strings/sys_string_conversions.h"
#include "ios/chrome/browser/main/browser.h"
#include "ios/web/public/thread/web_thread.h"
#include "ios/web/web_state/web_state_impl.h"

#include "net/base/mac/url_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark WebState

@interface WebState () {
  std::unique_ptr<brave::NativeWebState> web_state_;
}
@end

@implementation WebState
- (instancetype)initWithBrowser:(Browser*)browser
                 isOffTheRecord:(bool)isOffTheRecord {
  if ((self = [super init])) {
    web_state_ =
        std::make_unique<brave::NativeWebState>(browser, isOffTheRecord);
  }
  return self;
}

- (void)setTitle:(NSString*)title {
  DCHECK(web_state_.get());
  web_state_->SetTitle(base::SysNSStringToUTF16(title));
}

- (void)setURL:(NSURL*)url {
  DCHECK(web_state_.get());
  web_state_->SetURL(net::GURLWithNSURL(url));
}

- (base::WeakPtr<web::WebState>)internalWebState {
  DCHECK(web_state_.get());
  return web_state_->GetWeakWebState();
}
@end
