/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/web/web_state/ui/crw_web_controller.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

#include <Webkit/Webkit.h>

#define webViewNavigationProxy webViewNavigationProxy_ChromiumImpl
#include "src/ios/web/web_state/ui/crw_web_controller.mm"
#undef webViewNavigationProxy

#pragma mark - BackForwardList

// Back Forward List for use in Navigation Manager
@interface BackForwardList : NSObject
@property(nonatomic, readonly, copy) WKBackForwardListItem* currentItem;
@property(nonatomic, readonly, copy) NSArray<WKBackForwardListItem*>* backList;
@property(nonatomic, readonly, copy)
    NSArray<WKBackForwardListItem*>* forwardList;
@end

@implementation BackForwardList
@synthesize currentItem;
@synthesize backList;
@synthesize forwardList;

- (instancetype)init {
  self = [super init];
  return self;
}

- (WKBackForwardListItem*)itemAtIndex:(NSInteger)index {
  if (index == 0) {
    return currentItem;
  }

  if (index > 0 && forwardList.count) {
    return forwardList[index - 1];
  }

  if (backList.count) {
    return backList[backList.count + index];
  }

  return nullptr;
}

- (WKBackForwardListItem*)backItem {
  return backList.lastObject;
}

- (WKBackForwardListItem*)forwardItem {
  return forwardList.firstObject;
}
@end

#pragma mark - NavigationProxy

@interface NavigationProxy : NSObject <CRWWebViewNavigationProxy>
@end

@implementation NavigationProxy
- (instancetype)init {
  if ((self = [super init])) {
  }
  return self;
}

- (NSURL*)URL {
  return net::NSURLWithGURL(GURL(url::kAboutBlankURL));
}

- (WKBackForwardList*)backForwardList {
  return (WKBackForwardList*)[[BackForwardList alloc] init];
}

- (NSString*)title {
  return @"";
}
@end

#pragma mark - CRWWebController

@implementation CRWWebController (Brave)
- (id<CRWWebViewNavigationProxy>)webViewNavigationProxy {
  if (self.webView) {
    return [self webViewNavigationProxy_ChromiumImpl];
  }

  return [[NavigationProxy alloc] init];
}
@end
