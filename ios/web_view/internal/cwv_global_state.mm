// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// We completely replace the CWVGlobalState because we already provide global
// initialization with -[BraveCoreMain init].

#import "ios/web_view/public/cwv_global_state.h"

@implementation CWVEarlyInitFlags
@end

@implementation CWVGlobalState {
  NSString* _customUserAgent;
  NSString* _userAgentProduct;
}

+ (instancetype)sharedInstance {
  static CWVGlobalState* globalState = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    globalState = [[CWVGlobalState alloc] init];
  });
  return globalState;
}

- (NSString*)customUserAgent {
  return _customUserAgent;
}

- (void)setCustomUserAgent:(NSString*)customUserAgent {
  _customUserAgent = [customUserAgent copy];
}

- (NSString*)userAgentProduct {
  return _userAgentProduct;
}

- (void)setUserAgentProduct:(NSString*)userAgentProduct {
  _userAgentProduct = [userAgentProduct copy];
}

- (void)setGoogleAPIKey:(NSString*)googleAPIKey
               clientID:(NSString*)clientID
           clientSecret:(NSString*)clientSecret {
}

- (BOOL)isStarted {
  return YES;
}

- (BOOL)isEarlyInitialized {
  return YES;
}

- (void)earlyInit {
}

- (void)earlyInitWithFlags:(CWVEarlyInitFlags*)flags {
}

- (void)start {
}

- (void)stop {
}

@end
