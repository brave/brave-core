// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/strings/sys_string_conversions.h"
#import "brave/ios/testing/some_service.mojom.objc+private.h"

@implementation MojomSomeStruct (Private)

- (instancetype)initWithSomeStruct:(const ::SomeStructCpp&)obj {
  if ((self = [super init])) {
    self.value = base::SysUTF8ToNSString(obj.value);
    self.count = obj.count;
  }
  return self;
}

- (::SomeStructCpp)cppObjPtr {
  ::SomeStructCpp obj;
  obj.value = base::SysNSStringToUTF8(self.value);
  obj.count = self.count;
  return obj;
}

@end
