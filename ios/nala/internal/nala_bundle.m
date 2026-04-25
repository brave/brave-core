// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/nala/public/nala_bundle.h"

@interface NalaClassLookup : NSObject
@end

@implementation NalaClassLookup
@end

@implementation NSBundle (NalaAssets)

+ (instancetype)nalaAssetsBundle {
  return [NSBundle bundleForClass:NalaClassLookup.class];
}

@end
