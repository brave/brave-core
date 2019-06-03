// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import <Foundation/Foundation.h>
#import "Records.h"
#import "Enums.h"

NS_ASSUME_NONNULL_BEGIN

@class BATContributionInfo;

NS_SWIFT_NAME(PublisherInfo)
@interface BATPublisherInfo : NSObject
@property (nonatomic) NSString * id;
@property (nonatomic) unsigned long long duration;
@property (nonatomic) double score;
@property (nonatomic) unsigned int visits;
@property (nonatomic) unsigned int percent;
@property (nonatomic) double weight;
@property (nonatomic) BATPublisherExclude excluded;
@property (nonatomic) BATRewardsCategory category;
@property (nonatomic) unsigned long long reconcileStamp;
@property (nonatomic) bool verified;
@property (nonatomic) NSString * name;
@property (nonatomic) NSString * url;
@property (nonatomic) NSString * provider;
@property (nonatomic) NSString * faviconUrl;
@property (nonatomic) NSArray<BATContributionInfo *> * contributions;
@end


NS_ASSUME_NONNULL_END
