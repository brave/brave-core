/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "Enums.h"

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(ActivityInfoFilter.OrderPair)
@interface BATActivityInfoFilterOrderPair : NSObject
@property (nonatomic) NSString *propertyName;
@property (nonatomic, getter=isAscending) bool ascending;
@end

NS_SWIFT_NAME(ActivityInfoFilter)
@interface BATActivityInfoFilter : NSObject
@property (nonatomic) NSString *id;
@property (nonatomic) BATExcludeFilter excluded;
@property (nonatomic) unsigned long percent;
@property (nonatomic) NSArray<BATActivityInfoFilterOrderPair *> *orderBy;
@property (nonatomic) unsigned long long minDuration;
@property (nonatomic) unsigned long long reconcileStamp;
@property (nonatomic) bool nonVerified;
@property (nonatomic) unsigned long minVisits;
@end

NS_ASSUME_NONNULL_END
