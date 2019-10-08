/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>

@class PublisherInfo;

NS_ASSUME_NONNULL_BEGIN

@interface PendingContribution : NSManagedObject

+ (NSFetchRequest<PendingContribution *> *)fetchRequest;

@property (nonatomic) int64_t addedDate;
@property (nonatomic) double amount;
@property (nonatomic) int32_t type;
@property (nonatomic, copy) NSString *publisherID;
@property (nonatomic, copy) NSString *viewingID;
@property (nonatomic, retain) PublisherInfo *publisher;

@end

NS_ASSUME_NONNULL_END
