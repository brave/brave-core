/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <CoreData/CoreData.h>
#import <Foundation/Foundation.h>

@class ActivityInfo, ContributionInfo, RecurringDonation, PendingContribution;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface PublisherInfo : NSManagedObject

+ (NSFetchRequest<PublisherInfo*>*)fetchRequest;

@property(nonatomic) int32_t excluded;
@property(nonatomic, copy) NSString* faviconURL;
@property(nonatomic, copy) NSString* name;
@property(nonatomic, copy) NSString* provider;
@property(nonatomic, copy) NSString* publisherID;
@property(nonatomic, copy) NSString* url;
@property(nullable, nonatomic, retain) NSSet<ActivityInfo*>* activities;
@property(nullable, nonatomic, retain) NSSet<ContributionInfo*>* contributions;
@property(nullable, nonatomic, retain)
    NSSet<RecurringDonation*>* recurringDonations;
@property(nullable, nonatomic, retain)
    NSSet<PendingContribution*>* pendingContributions;

@end

@interface PublisherInfo (CoreDataGeneratedAccessors)

- (void)addActivitiesObject:(ActivityInfo*)value;
- (void)removeActivitiesObject:(ActivityInfo*)value;
- (void)addActivities:(NSSet<ActivityInfo*>*)values;
- (void)removeActivities:(NSSet<ActivityInfo*>*)values;

- (void)addContributionsObject:(ContributionInfo*)value;
- (void)removeContributionsObject:(ContributionInfo*)value;
- (void)addContributions:(NSSet<ContributionInfo*>*)values;
- (void)removeContributions:(NSSet<ContributionInfo*>*)values;

- (void)addRecurringDonationsObject:(RecurringDonation*)value;
- (void)removeRecurringDonationsObject:(RecurringDonation*)value;
- (void)addRecurringDonations:(NSSet<RecurringDonation*>*)values;
- (void)removeRecurringDonations:(NSSet<RecurringDonation*>*)values;

- (void)addPendingContributionsObject:(PendingContribution*)value;
- (void)removePendingContributionsObject:(PendingContribution*)value;
- (void)addPendingContributions:(NSSet<PendingContribution*>*)values;
- (void)removePendingContributions:(NSSet<PendingContribution*>*)values;

@end

NS_ASSUME_NONNULL_END
