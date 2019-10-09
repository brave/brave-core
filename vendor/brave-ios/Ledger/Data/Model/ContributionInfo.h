/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>

@class PublisherInfo;

NS_ASSUME_NONNULL_BEGIN

@interface ContributionInfo : NSManagedObject

+ (NSFetchRequest<ContributionInfo *> *)fetchRequest;

@property (nonatomic) int32_t type;
@property (nonatomic) int64_t date;
@property (nonatomic) int32_t month;
@property (nonatomic, copy) NSString *probi;
@property (nonatomic, copy) NSString *publisherID;
@property (nonatomic) int32_t year;
@property (nonatomic, retain) PublisherInfo *publisher;

@end

NS_ASSUME_NONNULL_END
