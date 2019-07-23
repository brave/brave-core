/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>

@class PublisherInfo;

NS_ASSUME_NONNULL_BEGIN

@interface ActivityInfo : NSManagedObject

+ (NSFetchRequest<ActivityInfo *> *)fetchRequest;

@property (nonatomic) int64_t duration;
@property (nonatomic) int32_t percent;
@property (nonatomic, copy) NSString *publisherID;
@property (nonatomic) int64_t reconcileStamp;
@property (nonatomic) double score;
@property (nonatomic) int32_t visits;
@property (nonatomic) double weight;
@property (nonatomic, retain) PublisherInfo *publisher;

@end

NS_ASSUME_NONNULL_END
