/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/ios/web_view/internal/cwv_web_view.mm"

@implementation CWVWebView (Internal)

- (web::WebState*)webState {
  return _webState.get();
}

+ (BOOL)_isRestoreDataValid:(NSData*)data {
  NSError* error = nil;
  NSKeyedUnarchiver* coder =
      [[NSKeyedUnarchiver alloc] initForReadingFromData:data error:&error];
  if (error) {
    return NO;
  }
  coder.requiresSecureCoding = NO;
  CWVWebViewProtobufStorage* cachedProtobufStorage =
      base::apple::ObjCCastStrict<CWVWebViewProtobufStorage>(
          [coder decodeObjectForKey:kProtobufStorageKey]);
  CRWSessionStorage* cachedSessionStorage =
      base::apple::ObjCCastStrict<CRWSessionStorage>(
          [coder decodeObjectForKey:kSessionStorageKey]);
  return (cachedProtobufStorage != nil || cachedSessionStorage != nil);
}

+ (id)objectFromValue:(const base::Value*)value {
  return NSObjectFromValue(value);
}

@end
