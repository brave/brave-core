/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BATLedgerDatabase.h"

#import "DataController.h"
#import "Model+CoreDataModel.h"

@implementation BATLedgerDatabase

+ (nullable __kindof NSManagedObject *)firstOfClass:(Class)clazz
                                    withPublisherID:(NSString *)publisherID
                                additionalPredicate:(nullable NSPredicate *)additionalPredicate
                                            context:(NSManagedObjectContext *)context
{
  const auto fetchRequest = [clazz fetchRequest];
  fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(clazz)
                                    inManagedObjectContext:context];
  fetchRequest.fetchLimit = 1;
  
  const auto predicates = [[NSMutableArray<NSPredicate *> alloc] init];
  [predicates addObject:[NSPredicate predicateWithFormat:@"publisherID == %@", publisherID]];
  
  if (additionalPredicate) {
    [predicates addObject:additionalPredicate];
  }
  
  fetchRequest.predicate = [NSCompoundPredicate andPredicateWithSubpredicates:predicates];
  
  NSError *error;
  const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
  if (error) {
    NSLog(@"%s: %@", __PRETTY_FUNCTION__, error);
  }
  
  return fetchedObjects.firstObject;
}

+ (nullable PublisherInfo *)getPublisherInfoWithID:(NSString *)publisherID context:(NSManagedObjectContext *)context
{
  return [self firstOfClass:PublisherInfo.class withPublisherID:publisherID
            additionalPredicate:nil context:context];
}

+ (nullable ActivityInfo *)getActivityInfoWithPublisherID:(NSString *)publisherID
                                           reconcileStamp:(unsigned long long)reconcileStamp
                                                  context:(NSManagedObjectContext *)context
{
  const auto reconcilePredicate = [NSPredicate predicateWithFormat:@"reconcileStamp == %ld", reconcileStamp];
  return [self firstOfClass:ActivityInfo.class withPublisherID:publisherID
            additionalPredicate:reconcilePredicate context:context];
}

+ (nullable RecurringDonation *)getRecurringDonationWithPublisherID:(NSString *)publisherID context:(NSManagedObjectContext *)context
{
  return [self firstOfClass:RecurringDonation.class withPublisherID:publisherID
            additionalPredicate:nil context:context];
}

+ (nullable MediaPublisherInfo *)getMediaPublisherInfoWithMediaKey:(NSString *)mediaKey context:(NSManagedObjectContext *)context
{
  const auto fetchRequest = MediaPublisherInfo.fetchRequest;
  fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(MediaPublisherInfo.class)
                                    inManagedObjectContext:context];
  fetchRequest.fetchLimit = 1;
  fetchRequest.predicate = [NSPredicate predicateWithFormat:@"mediaKey == %@", mediaKey];
  
  NSError *error;
  const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
  if (error) {
    NSLog(@"%s: %@", __PRETTY_FUNCTION__, error);
  }
  
  return fetchedObjects.firstObject;
}

#pragma mark - Publisher Info

+ (BATPublisherInfo *)publisherInfoWithPublisherID:(NSString *)publisherID
{
  auto databaseInfo = [self getPublisherInfoWithID:publisherID context:DataController.viewContext];
  if (!databaseInfo) {
    return nil;
  }
  auto info = [[BATPublisherInfo alloc] init];
  info.id = databaseInfo.publisherID;
  info.name = databaseInfo.name;
  info.url = databaseInfo.url.absoluteString;
  info.faviconUrl = databaseInfo.faviconURL.absoluteString;
  info.provider = databaseInfo.provider;
  info.verified = databaseInfo.verified;
  info.excluded = (BATPublisherExclude)databaseInfo.excluded;
  return info;
}

+ (BATPublisherInfo *)panelPublisherWithFilter:(BATActivityInfoFilter *)filter
{
  const auto info = [self publisherInfoWithPublisherID:filter.id];
  const auto activity = [self getActivityInfoWithPublisherID:filter.id
                                              reconcileStamp:filter.reconcileStamp
                                                     context:DataController.viewContext];
  if (activity) {
    info.percent = activity.percent;
  }
  return info;
}

+ (void)insertOrUpdatePublisherInfo:(BATPublisherInfo *)info
{
  return [self insertOrUpdatePublisherInfo:info context:nil];
}

+ (void)insertOrUpdatePublisherInfo:(BATPublisherInfo *)info context:(nullable NSManagedObjectContext *)context
{
  if (info.id.length == 0) {
    return;
  }
  
  [DataController.shared performOnContext:context task:^(NSManagedObjectContext * _Nonnull context) {
    const auto pi = [self getPublisherInfoWithID:info.id context:context] ?:
      [[PublisherInfo alloc] initWithEntity:PublisherInfo.entity
             insertIntoManagedObjectContext:context];;
    pi.publisherID = info.id;
    pi.verified = info.verified;
    pi.excluded = info.excluded;
    pi.name = info.name;
    pi.url = [NSURL URLWithString:info.url];
    pi.provider = info.provider;
    pi.faviconURL = [NSURL URLWithString:info.faviconUrl];
  }];
}

+ (void)restoreExcludedPublishers
{
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    const auto fetchRequest = PublisherInfo.fetchRequest;
    fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(PublisherInfo.class)
                                      inManagedObjectContext:context];
    fetchRequest.predicate = [NSPredicate predicateWithFormat:@"excluded == %d", BATPublisherExcludeExcluded];
    
    NSError *error;
    const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
    
    if (error) {
      NSLog(@"%s: %@", __PRETTY_FUNCTION__, error);
    }
    
    for (PublisherInfo *info in fetchedObjects) {
      info.excluded = BATPublisherExcludeDefault;
    }
  }];
}

+ (NSUInteger)excludedPublishersCount
{
  const auto context = DataController.viewContext;
  const auto fetchRequest = ActivityInfo.fetchRequest;
  fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(PublisherInfo.class)
                                    inManagedObjectContext:context];
  fetchRequest.predicate = [NSPredicate predicateWithFormat:@"excluded == %d", BATPublisherExcludeExcluded];
  
  NSError *error;
  const auto count = [context countForFetchRequest:fetchRequest error:&error];
  if (error) {
    NSLog(@"%s: %@", __PRETTY_FUNCTION__, error);
  }
  return count;
}

#pragma mark - Contribution Info

+ (void)insertContributionInfo:(NSString *)probi month:(const int)month year:(const int)year date:(const uint32_t)date publisherKey:(NSString *)publisherKey category:(BATRewardsCategory)category
{
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    auto ci = [[ContributionInfo alloc] initWithEntity:ContributionInfo.entity
                        insertIntoManagedObjectContext:context];
    ci.probi = probi;
    ci.month = month;
    ci.year = year;
    ci.date = date;
    ci.publisherID = publisherKey;
    ci.category = category;
    ci.publisher = [self getPublisherInfoWithID:publisherKey context:context];
  }];
}

+ (NSArray<BATPublisherInfo *> *)oneTimeTipsPublishersForMonth:(BATActivityMonth)month year:(int)year
{
  const auto context = DataController.viewContext;
  const auto fetchRequest = ContributionInfo.fetchRequest;
  fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(ContributionInfo.class)
                                    inManagedObjectContext:context];
  fetchRequest.predicate = [NSPredicate predicateWithFormat:@"month = %d AND year = %d AND category = %d",
                            month, year, BATRewardsCategoryOneTimeTip];
  
  NSError *error;
  const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
  if (error) {
    NSLog(@"%@", error);
  }
  
  const auto publishers = [[NSMutableArray<BATPublisherInfo *> alloc] init];
  for (ContributionInfo *ci in fetchedObjects) {
    auto info = [[BATPublisherInfo alloc] init];
    info.id = ci.publisherID;
    info.name = ci.publisher.name;
    info.url = ci.publisher.url.absoluteString;
    info.faviconUrl = ci.publisher.faviconURL.absoluteString;
    info.weight = [ci.probi doubleValue];
    info.reconcileStamp = ci.date;
    info.verified = ci.publisher.verified;
    info.provider = ci.publisher.provider;
    [publishers addObject:info];
  }
  return publishers;
}

#pragma mark - Activity Info

+ (void)insertOrUpdateActivityInfoFromPublisher:(BATPublisherInfo *)info
{
  [self insertOrUpdateActivityInfoFromPublisher:info context:nil];
}

+ (void)insertOrUpdateActivityInfoFromPublisher:(BATPublisherInfo *)info  context:(nullable NSManagedObjectContext *)context
{
  if (info.id.length == 0) {
    return;
  }
  
  [DataController.shared performOnContext:context task:^(NSManagedObjectContext * _Nonnull context) {
    [self insertOrUpdatePublisherInfo:info context:context];
    
    const auto publisherFromInfo = [self getActivityInfoWithPublisherID:info.id
                                                         reconcileStamp:info.reconcileStamp context:context];
    
    const auto ai = publisherFromInfo ?:
      [[ActivityInfo alloc] initWithEntity:ActivityInfo.entity insertIntoManagedObjectContext:context];
    
    ai.publisher = [self getPublisherInfoWithID:info.id context:context];
    ai.publisherID = info.id;
    ai.duration = info.duration;
    ai.score = info.score;
    ai.percent = info.percent;
    ai.weight = info.weight;
    ai.reconcileStamp = info.reconcileStamp;
    ai.visits = info.visits;
  }];
}

+ (void)insertOrUpdateActivitiesInfoFromPublishers:(NSArray<BATPublisherInfo *> *)publishers
{
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    for (BATPublisherInfo *info in publishers) {
      [self insertOrUpdateActivityInfoFromPublisher:info context:context];
    }
  }];
}

+ (NSArray<BATPublisherInfo *> *)publishersWithActivityFromOffset:(uint32_t)start limit:(uint32_t)limit filter:(BATActivityInfoFilter *)filter
{
  const auto context = DataController.viewContext;
  const auto fetchRequest = ActivityInfo.fetchRequest;
  fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(ActivityInfo.class)
                                    inManagedObjectContext:context];
  if (limit > 0) {
    fetchRequest.fetchLimit = limit;
    if (start > 1) {
      fetchRequest.fetchOffset = start;
    }
  }
  
  NSMutableArray *sortDescriptors = [[NSMutableArray alloc] init];
  for (BATActivityInfoFilterOrderPair *orderPair in filter.orderBy) {
    [sortDescriptors addObject:[NSSortDescriptor sortDescriptorWithKey:orderPair.propertyName ascending:orderPair.ascending]];
  }
  fetchRequest.sortDescriptors = sortDescriptors;
  
  const auto predicates = [[NSMutableArray<NSPredicate *> alloc] init];
  
  if (filter.id.length > 0) {
    [predicates addObject:[NSPredicate predicateWithFormat:@"publisherID == %@", filter.id]];
  }
  
  if (filter.reconcileStamp > 0) {
    [predicates addObject:[NSPredicate predicateWithFormat:@"reconcileStamp == %ld", filter.reconcileStamp]];
  }
  
  if (filter.minDuration > 0) {
    [predicates addObject:[NSPredicate predicateWithFormat:@"duration >= %ld", filter.minDuration]];
  }
  
  if (filter.excluded != BATExcludeFilterFilterAll) {
    if (filter.excluded != BATExcludeFilterFilterAllExceptExcluded) {
      [predicates addObject:[NSPredicate predicateWithFormat:@"publisher.excluded == %d", filter.excluded]];
    } else {
      [predicates addObject:[NSPredicate predicateWithFormat:@"publisher.excluded != %d", BATExcludeFilterFilterExcluded]];
    }
  }
  
  if (filter.percent) {
    [predicates addObject:[NSPredicate predicateWithFormat:@"percent >= %d", filter.percent]];
  }
  
  if (filter.minVisits) {
    [predicates addObject:[NSPredicate predicateWithFormat:@"visits >= %d", filter.minVisits]];
  }
  
  if (predicates.count > 0) {
    fetchRequest.predicate = [NSCompoundPredicate andPredicateWithSubpredicates:predicates];
  }
  
  NSError *error;
  const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
  if (error) {
    NSLog(@"%@", error);
  }
  
  const auto publishers = [[NSMutableArray<BATPublisherInfo *> alloc] init];
  for (ActivityInfo *activity in fetchedObjects) {
    auto info = [[BATPublisherInfo alloc] init];
    info.id = activity.publisherID;
    info.score = activity.score;
    info.percent = activity.percent;
    info.weight = activity.weight;
    info.verified = activity.publisher.verified;
    info.excluded = (BATPublisherExclude)activity.publisher.excluded;
    info.name = activity.publisher.name;
    info.url = activity.publisher.url.absoluteString;
    info.provider = activity.publisher.provider;
    info.faviconUrl = activity.publisher.faviconURL.absoluteString;
    info.reconcileStamp = activity.reconcileStamp;
    info.visits = activity.visits;
    [publishers addObject:info];
  }
  return publishers;
}

+ (void)deleteActivityInfoWithPublisherID:(NSString *)publisherID reconcileStamp:(uint64_t)reconcileStamp
{
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    const auto request = ActivityInfo.fetchRequest;
    request.entity = [NSEntityDescription entityForName:NSStringFromClass(ActivityInfo.class)
                                 inManagedObjectContext:context];
    request.predicate = [NSPredicate predicateWithFormat:@"publisherID == %@ AND reconcileStamp == %ld", publisherID, reconcileStamp];
    
    NSError *error;
    const auto fetchedObjects = [context executeFetchRequest:request error:&error];
    if (error) {
      NSLog(@"%@", error);
    }
    
    for (ActivityInfo *info in fetchedObjects) {
      [context deleteObject:info];
    }
  }];
}

#pragma mark - Media Publisher Info

+ (BATPublisherInfo *)mediaPublisherInfoWithMediaKey:(NSString *)mediaKey
{
  const auto mi = [self getMediaPublisherInfoWithMediaKey:mediaKey context:DataController.viewContext];
  if (!mi) {
    return nil;
  }
  // As far as I know, there is no data that is actually coming from MediaPublisherInfo, just basically
  // here to grab a publisher info object with a media key
  return [self publisherInfoWithPublisherID:mi.publisherID];
}

+ (void)insertOrUpdateMediaPublisherInfoWithMediaKey:(NSString *)mediaKey publisherID:(NSString *)publisherID
{
  if (mediaKey.length == 0 || publisherID.length == 0) {
    return;
  }
  
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    auto mi = [self getMediaPublisherInfoWithMediaKey:mediaKey context:context] ?: [[MediaPublisherInfo alloc] initWithEntity:MediaPublisherInfo.entity
                          insertIntoManagedObjectContext:context];
    mi.mediaKey = mediaKey;
    mi.publisherID = publisherID;
  }];
}

#pragma mark - Recurring Tips

+ (NSArray<BATPublisherInfo *> *)recurringTips
{
  const auto context = DataController.viewContext;
  const auto fetchRequest = RecurringDonation.fetchRequest;
  fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(RecurringDonation.class)
                                    inManagedObjectContext:context];
  
  NSError *error;
  const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
  if (error) {
    NSLog(@"%@", error);
  }
  
  const auto publishers = [[NSMutableArray<BATPublisherInfo *> alloc] init];
  for (RecurringDonation *rd in fetchedObjects) {
    auto info = [[BATPublisherInfo alloc] init];
    info.id = rd.publisherID;
    info.name = rd.publisher.name;
    info.url = rd.publisher.url.absoluteString;
    info.faviconUrl = rd.publisher.faviconURL.absoluteString;
    info.weight = rd.amount;
    info.reconcileStamp = rd.addedDate;
    info.verified = rd.publisher.verified;
    info.provider = rd.publisher.provider;
    [publishers addObject:info];
  }
  return publishers;
}

+ (void)insertOrUpdateRecurringTipWithPublisherID:(NSString *)publisherID
                                           amount:(double)amount
                                        dateAdded:(uint32_t)dateAdded
{
  if (publisherID.length == 0) {
    return;
  }
  
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    auto rd = [self getRecurringDonationWithPublisherID:publisherID context:context] ?: [[RecurringDonation alloc] initWithEntity:RecurringDonation.entity
                         insertIntoManagedObjectContext:context];
    rd.publisherID = publisherID;
    rd.amount = amount;
    rd.addedDate = dateAdded;
    rd.publisher = [self getPublisherInfoWithID:publisherID context:context];
  }];
}

+ (BOOL)removeRecurringTipWithPublisherID:(NSString *)publisherID
{
  // Early guard to check if object exists.
  // The check happens on main thread, while the deletion is done in background.
  const auto donationExists = [self getRecurringDonationWithPublisherID:publisherID
                                                                context:DataController.viewContext];
  if (!donationExists) {
    return NO;
  }
  
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    const auto donation = [self getRecurringDonationWithPublisherID:publisherID context:context];
    [context deleteObject:donation];
  }];
  return YES;
}

#pragma mark - Pending Contributions

+ (void)insertPendingContributions:(NSArray<BATPendingContribution *> *)contributions
{
  const auto now = [[NSDate date] timeIntervalSince1970];
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    for (BATPendingContribution *contribution in contributions) {
      auto pc = [[PendingContribution alloc] initWithEntity:PendingContribution.entity
                             insertIntoManagedObjectContext:context];
      pc.publisherID = contribution.publisherKey;
      pc.amount = contribution.amount;
      pc.addedDate = now;
      pc.viewingID = contribution.viewingId;
      pc.category = contribution.category;
    }
  }];
}

+ (double)reservedAmountForPendingContributions
{
  const auto context = DataController.viewContext;
  const auto fetchRequest = PendingContribution.fetchRequest;
  fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(PendingContribution.class)
                                    inManagedObjectContext:context];
  
  NSError *error;
  const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
  if (!fetchedObjects) {
    NSLog(@"%@", error);
  }
  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-messaging-id"
  return [[fetchedObjects valueForKeyPath:@"@sum.amount"] doubleValue];
#pragma clang diagnostic pop
}

@end
