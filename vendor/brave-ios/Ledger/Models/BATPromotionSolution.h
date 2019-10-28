// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// The solution to claiming a promotion on iOS. Obtain the `noonce` through
/// `[BATBraveLedger claimPromotion:completion:]` method, and obtain the
/// blob and signature from the users keychain
NS_SWIFT_NAME(PromotionSolution)
@interface BATPromotionSolution : NSObject

@property (nonatomic, copy) NSString *noonce;
@property (nonatomic, copy) NSString *blob;
@property (nonatomic, copy) NSString *signature;

- (NSString *)JSONPayload;

@end

NS_ASSUME_NONNULL_END
