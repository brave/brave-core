/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificateExtensionGeneralNameModel.h"

@implementation BraveCertificateExtensionGeneralNameModel
- (instancetype)init {
  if ((self = [super init])) {
    _type = BraveGeneralNameType_INVALID;
    _other = [[NSString alloc] init];
    _nameAssigner = [[NSString alloc] init];
    _partyName = [[NSString alloc] init];
    _dirName = @{};
  }
  return self;
}
@end
