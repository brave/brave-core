// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/strings/sys_string_conversions.h"
#import "brave/ios/testing/some_service.mojom.objc+private.h"

@implementation MojomSomeStruct (SomeStructCppBridge)

- (NSString*)value {
  return base::SysUTF8ToNSString(self.cppObjPtr.value);
}

- (void)setValue:(NSString*)value {
  [self mutableCppObjRef].value = base::SysNSStringToUTF8(value);
}

- (int32_t)count {
  return self.cppObjPtr.count;
}

- (void)setCount:(int32_t)count {
  [self mutableCppObjRef].count = count;
}

@end
