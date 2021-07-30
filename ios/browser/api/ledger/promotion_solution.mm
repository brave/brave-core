/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "promotion_solution.h"

#include "base/logging.h"
#import "ledger.mojom.objc.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation PromotionSolution

- (NSString*)JSONPayload {
  NSDictionary* payload = @{
    @"nonce" : self.nonce,
    @"blob" : self.blob,
    @"signature" : self.signature
  };
  NSData* jsonData = [NSJSONSerialization dataWithJSONObject:payload
                                                     options:0
                                                       error:nil];
  if (!jsonData) {
    LOG(INFO) << "Missing JSON payload while attempting to attest promotion";
    return @"";
  }
  return [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
}

@end
