/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/wallet/brave_wallet_api.h"
#import "brave/ios/browser/api/wallet/keyring_controller_ios+private.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BraveWalletAPI

- (instancetype)init {
  if ((self = [super init])) {
  }
  return self;
}

@end
