/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/notimplemented.h"
#include "ios/web/public/web_state.h"

namespace ios_web_view {
void AttachTabHelpers(web::WebState* web_state);
}

#define BRAVE_NOP_SERVICE \
  NOTIMPLEMENTED();       \
  return nil;
#define BRAVE_ATTACH_TAB_HELPERS \
  ios_web_view::AttachTabHelpers(_webState.get());
#include "src/ios/web_view/internal/cwv_web_view.mm"
#undef BRAVE_ATTACH_TAB_HELPERS
#undef BRAVE_NOP_SERVICE

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

@end
