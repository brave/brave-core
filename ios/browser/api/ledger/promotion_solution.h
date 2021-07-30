/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_LEDGER_PROMOTION_SOLUTION_H_
#define BRAVE_IOS_BROWSER_API_LEDGER_PROMOTION_SOLUTION_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// The solution to claiming a promotion on iOS. Obtain the `nonce` through
/// `[BraveLedger claimPromotion:completion:]` method, and obtain the
/// blob and signature from the users keychain
OBJC_EXPORT
NS_SWIFT_NAME(PromotionSolution)
@interface PromotionSolution : NSObject

@property(nonatomic, copy) NSString* nonce;
@property(nonatomic, copy) NSString* blob;
@property(nonatomic, copy) NSString* signature;

- (NSString*)JSONPayload;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_LEDGER_PROMOTION_SOLUTION_H_
