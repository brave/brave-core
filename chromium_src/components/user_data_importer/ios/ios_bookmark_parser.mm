// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import <Foundation/Foundation.h>

// This override replaces the location where the bookmark parser looks for
// `bookmark_parser.js` which they've hardcoded to the main bundle but we copy
// it into the framework.

#include "base/apple/bundle_locations.h"

@interface NSBundle (Brave)
+ (NSBundle*)brave_frameworkBundle;
@end

#define mainBundle brave_frameworkBundle
#include <components/user_data_importer/ios/ios_bookmark_parser.mm>
#undef mainBundle

@implementation NSBundle (Brave)
+ (NSBundle*)brave_frameworkBundle {
  return base::apple::FrameworkBundle();
}
@end
