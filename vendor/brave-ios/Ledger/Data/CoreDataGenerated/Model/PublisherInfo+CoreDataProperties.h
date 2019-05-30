//
//  PublisherInfo+CoreDataProperties.h
//  
//
//  Created by Kyle Hickinson on 2019-05-24.
//
//  This file was automatically generated and should not be edited.
//

#import "PublisherInfo+CoreDataClass.h"


NS_ASSUME_NONNULL_BEGIN

@interface PublisherInfo (CoreDataProperties)

+ (NSFetchRequest<PublisherInfo *> *)fetchRequest;

@property (nonatomic) int32_t excluded;
@property (nullable, nonatomic, copy) NSURL *faviconURL;
@property (nullable, nonatomic, copy) NSString *name;
@property (nullable, nonatomic, copy) NSString *provider;
@property (nullable, nonatomic, copy) NSString *publisherID;
@property (nullable, nonatomic, copy) NSURL *url;
@property (nonatomic) BOOL verified;
@property (nullable, nonatomic, retain) NSSet<ActivityInfo *> *activities;
@property (nullable, nonatomic, retain) NSSet<ContributionInfo *> *contributions;
@property (nullable, nonatomic, retain) NSSet<RecurringDonation *> *recurringDonations;

@end

@interface PublisherInfo (CoreDataGeneratedAccessors)

- (void)addActivitiesObject:(ActivityInfo *)value;
- (void)removeActivitiesObject:(ActivityInfo *)value;
- (void)addActivities:(NSSet<ActivityInfo *> *)values;
- (void)removeActivities:(NSSet<ActivityInfo *> *)values;

- (void)addContributionsObject:(ContributionInfo *)value;
- (void)removeContributionsObject:(ContributionInfo *)value;
- (void)addContributions:(NSSet<ContributionInfo *> *)values;
- (void)removeContributions:(NSSet<ContributionInfo *> *)values;

- (void)addRecurringDonationsObject:(RecurringDonation *)value;
- (void)removeRecurringDonationsObject:(RecurringDonation *)value;
- (void)addRecurringDonations:(NSSet<RecurringDonation *> *)values;
- (void)removeRecurringDonations:(NSSet<RecurringDonation *> *)values;

@end

NS_ASSUME_NONNULL_END
