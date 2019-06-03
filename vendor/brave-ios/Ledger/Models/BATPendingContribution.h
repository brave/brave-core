// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import <Foundation/Foundation.h>
#import "Enums.h"

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(PendingContribution)
@interface BATPendingContribution : NSObject
@property (nonatomic) NSString * publisherKey;
@property (nonatomic) double amount;
@property (nonatomic) unsigned long long addedDate;
@property (nonatomic) NSString * viewingId;
@property (nonatomic) BATRewardsCategory category;
@end


NS_ASSUME_NONNULL_END
