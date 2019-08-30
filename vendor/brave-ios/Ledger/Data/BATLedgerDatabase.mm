/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BATLedgerDatabase.h"

#import "DataController.h"
#import "CoreDataModels.h"
#import "bat/ledger/publisher_info.h"

NS_INLINE DataControllerCompletion _Nullable
WriteToDataControllerCompletion(BATLedgerDatabaseWriteCompletion _Nullable completion) {
  if (!completion) {
    return nil;
  }
  return ^(NSError * _Nullable error) {
    dispatch_async(dispatch_get_main_queue(), ^{
      completion(error == nil);
    });
  };
}

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

+ (nullable PendingContribution *)getPendingContributonWithPublisherID:(NSString *)publisherID
                                                           viewingID:(NSString *)viewingID
                                                           addedDate:(UInt64)addedDate
                                                             context:(NSManagedObjectContext *)context
{
  const auto otherPedicate = [NSPredicate predicateWithFormat:@"viewingID == %@ && addedDate = %d", viewingID, addedDate];
  return [self firstOfClass:PendingContribution.class withPublisherID:publisherID
        additionalPredicate:otherPedicate context:context];
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

+ (nullable ServerPublisherInfo *)getServerPublisherInfoWithPublisherID:(NSString *)publisherID context:(NSManagedObjectContext *)context
{
  return [self firstOfClass:ServerPublisherInfo.class withPublisherID:publisherID additionalPredicate:nil context:context];
}

+ (nullable ServerPublisherAmount *)getServerPublisherAmountWithPublisherID:(NSString *)publisherID amount:(double)amount context:(NSManagedObjectContext *)context
{
  return [self firstOfClass:ServerPublisherAmount.class withPublisherID:publisherID additionalPredicate:[NSPredicate predicateWithFormat:@"amount == %lf", amount] context:context];
}

+ (nullable ServerPublisherBanner *)getServerPublisherBannerWithPublisherID:(NSString *)publisherID context:(NSManagedObjectContext *)context
{
  return [self firstOfClass:ServerPublisherBanner.class withPublisherID:publisherID additionalPredicate:nil context:context];
}

+ (nullable ServerPublisherLink *)getServerPublisherLinkWithPublisherID:(NSString *)publisherID provider:(NSString *)provider context:(NSManagedObjectContext *)context
{
  return [self firstOfClass:ServerPublisherLink.class withPublisherID:publisherID additionalPredicate:[NSPredicate predicateWithFormat:@"provider == %@", provider] context:context];
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
  info.url = databaseInfo.url;
  info.faviconUrl = databaseInfo.faviconURL;
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

+ (void)insertOrUpdatePublisherInfo:(BATPublisherInfo *)info completion:( BATLedgerDatabaseWriteCompletion)completion
{
  return [self insertOrUpdatePublisherInfo:info context:nil completion:completion];
}

+ (void)insertOrUpdatePublisherInfo:(BATPublisherInfo *)info context:(nullable NSManagedObjectContext *)context completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  if (info.id.length == 0) {
    if (completion) { completion(NO); }
    return;
  }

  [DataController.shared performOnContext:context task:^(NSManagedObjectContext * _Nonnull context) {
    const auto pi = [self getPublisherInfoWithID:info.id context:context] ?:
      [[PublisherInfo alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(PublisherInfo.class) inManagedObjectContext:context]
             insertIntoManagedObjectContext:context];
    pi.publisherID = info.id;
    pi.verified = info.verified;
    pi.excluded = info.excluded;
    pi.name = info.name;
    pi.url = info.url;
    pi.provider = info.provider;
    if ([info.faviconUrl isEqualToString:[NSString stringWithUTF8String:ledger::kClearFavicon]]) {
      pi.faviconURL = @"";
    } else {
      pi.faviconURL = info.faviconUrl;
    }
  } completion:WriteToDataControllerCompletion(completion)];
}

+ (NSArray<BATPublisherInfo *> *)excludedPublishers
{
  const auto context = DataController.viewContext;
  const auto fetchRequest = PublisherInfo.fetchRequest;
  fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(PublisherInfo.class)
                                    inManagedObjectContext:context];
  fetchRequest.predicate = [NSPredicate predicateWithFormat:@"excluded == %d", BATExcludeFilterFilterExcluded];

  NSError *error;
  const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
  if (error) {
    NSLog(@"%@", error);
  }

  const auto publishers = [[NSMutableArray<BATPublisherInfo *> alloc] init];
  for (PublisherInfo *publisher in fetchedObjects) {
    auto info = [[BATPublisherInfo alloc] init];
    info.id = publisher.publisherID;
    info.excluded = static_cast<BATPublisherExclude>(publisher.excluded);
    info.name = publisher.name;
    info.url = publisher.url;
    info.provider = publisher.provider;
    info.faviconUrl = publisher.faviconURL;
    [publishers addObject:info];
  }
  return publishers;
}

+ (void)restoreExcludedPublishers:(nullable BATLedgerDatabaseWriteCompletion)completion
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
  } completion:WriteToDataControllerCompletion(completion)];
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

+ (void)insertContributionInfo:(NSString *)probi month:(const int)month year:(const int)year date:(const uint32_t)date publisherKey:(NSString *)publisherKey category:(BATRewardsCategory)category completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    auto ci = [[ContributionInfo alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(ContributionInfo.class) inManagedObjectContext:context]
                        insertIntoManagedObjectContext:context];
    ci.probi = probi;
    ci.month = month;
    ci.year = year;
    ci.date = date;
    ci.publisherID = publisherKey;
    ci.category = category;
    ci.publisher = [self getPublisherInfoWithID:publisherKey context:context];
  } completion:WriteToDataControllerCompletion(completion)];
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
    info.url = ci.publisher.url;
    info.faviconUrl = ci.publisher.faviconURL;
    info.weight = [ci.probi doubleValue];
    info.reconcileStamp = ci.date;
    info.verified = ci.publisher.verified;
    info.provider = ci.publisher.provider;
    [publishers addObject:info];
  }
  return publishers;
}

#pragma mark - Activity Info

+ (void)insertOrUpdateActivityInfoFromPublisher:(BATPublisherInfo *)info completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  [self insertOrUpdateActivityInfoFromPublisher:info context:nil completion:completion];
}

+ (void)insertOrUpdateActivityInfoFromPublisher:(BATPublisherInfo *)info context:(nullable NSManagedObjectContext *)context completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  if (info.id.length == 0) {
    if (completion) { completion(NO); }
    return;
  }

  [DataController.shared performOnContext:context task:^(NSManagedObjectContext * _Nonnull context) {
    [self insertOrUpdatePublisherInfo:info context:context completion:nil];

    const auto publisherFromInfo = [self getActivityInfoWithPublisherID:info.id
                                                         reconcileStamp:info.reconcileStamp context:context];

    const auto ai = publisherFromInfo ?:
      [[ActivityInfo alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(ActivityInfo.class) inManagedObjectContext:context] insertIntoManagedObjectContext:context];

    ai.publisher = [self getPublisherInfoWithID:info.id context:context];
    ai.publisherID = info.id;
    ai.duration = info.duration;
    ai.score = info.score;
    ai.percent = info.percent;
    ai.weight = info.weight;
    ai.reconcileStamp = info.reconcileStamp;
    ai.visits = info.visits;
  } completion:WriteToDataControllerCompletion(completion)];
}

+ (void)insertOrUpdateActivitiesInfoFromPublishers:(NSArray<BATPublisherInfo *> *)publishers  completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    for (BATPublisherInfo *info in publishers) {
      [self insertOrUpdateActivityInfoFromPublisher:info context:context completion:nil];
    }
  } completion:WriteToDataControllerCompletion(completion)];
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
    info.duration = activity.duration;
    info.score = activity.score;
    info.percent = activity.percent;
    info.weight = activity.weight;
    info.verified = activity.publisher.verified;
    info.excluded = (BATPublisherExclude)activity.publisher.excluded;
    info.name = activity.publisher.name;
    info.url = activity.publisher.url;
    info.provider = activity.publisher.provider;
    info.faviconUrl = activity.publisher.faviconURL;
    info.reconcileStamp = activity.reconcileStamp;
    info.visits = activity.visits;
    [publishers addObject:info];
  }
  return publishers;
}

+ (void)deleteActivityInfoWithPublisherID:(NSString *)publisherID reconcileStamp:(uint64_t)reconcileStamp completion:(nullable BATLedgerDatabaseWriteCompletion)completion
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
      dispatch_async(dispatch_get_main_queue(), ^{
        completion(NO);
      });
      return;
    }

    for (ActivityInfo *info in fetchedObjects) {
      [context deleteObject:info];
    }
  } completion:WriteToDataControllerCompletion(completion)];
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

+ (void)insertOrUpdateMediaPublisherInfoWithMediaKey:(NSString *)mediaKey publisherID:(NSString *)publisherID completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  if (mediaKey.length == 0 || publisherID.length == 0) {
    if (completion) { completion(NO); }
    return;
  }

  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    auto mi = [self getMediaPublisherInfoWithMediaKey:mediaKey context:context] ?: [[MediaPublisherInfo alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(MediaPublisherInfo.class) inManagedObjectContext:context]
                          insertIntoManagedObjectContext:context];
    mi.mediaKey = mediaKey;
    mi.publisherID = publisherID;
  } completion:WriteToDataControllerCompletion(completion)];
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
    info.url = rd.publisher.url;
    info.faviconUrl = rd.publisher.faviconURL;
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
                                       completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  if (publisherID.length == 0) {
    if (completion) { completion(NO); }
    return;
  }

  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    auto rd = [self getRecurringDonationWithPublisherID:publisherID context:context] ?: [[RecurringDonation alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(RecurringDonation.class) inManagedObjectContext:context]
                         insertIntoManagedObjectContext:context];
    rd.publisherID = publisherID;
    rd.amount = amount;
    rd.addedDate = dateAdded;
    rd.publisher = [self getPublisherInfoWithID:publisherID context:context];
  } completion:WriteToDataControllerCompletion(completion)];
}

+ (void)removeRecurringTipWithPublisherID:(NSString *)publisherID completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  // Early guard to check if object exists.
  // The check happens on main thread, while the deletion is done in background.
  const auto donationExists = [self getRecurringDonationWithPublisherID:publisherID
                                                                context:DataController.viewContext];
  if (!donationExists) {
    if (completion) { completion(NO); }
    return;
  }

  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    const auto donation = [self getRecurringDonationWithPublisherID:publisherID context:context];
    [context deleteObject:donation];
  } completion:WriteToDataControllerCompletion(completion)];
}

#pragma mark - Pending Contributions

+ (NSArray<BATPendingContributionInfo *> *)pendingContributions
{
  const auto context = DataController.viewContext;
  const auto fetchRequest = PendingContribution.fetchRequest;
  fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(PendingContribution.class)
                                    inManagedObjectContext:context];

  NSError *error;
  const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
  if (error) {
    NSLog(@"%@", error);
  }

  const auto publishers = [[NSMutableArray<BATPendingContributionInfo *> alloc] init];
  for (PendingContribution *pc in fetchedObjects) {
    auto info = [[BATPendingContributionInfo alloc] init];
    info.publisherKey = pc.publisherID;
    info.name = pc.publisher.name;
    info.url = pc.publisher.url;
    info.faviconUrl = pc.publisher.faviconURL;
    info.verified = pc.publisher.verified;
    info.provider = pc.publisher.provider;
    info.amount = pc.amount;
    info.addedDate = pc.addedDate;
    info.viewingId = pc.viewingID;
    info.category = pc.category;
    [publishers addObject:info];
  }
  return publishers;
}

+ (void)insertPendingContributions:(NSArray<BATPendingContribution *> *)contributions completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  const auto now = [[NSDate date] timeIntervalSince1970];
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    for (BATPendingContribution *contribution in contributions) {
      auto pc = [[PendingContribution alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(PendingContribution.class) inManagedObjectContext:context]
                             insertIntoManagedObjectContext:context];
      pc.publisher = [self getPublisherInfoWithID:contribution.publisherKey context:context];
      pc.publisherID = contribution.publisherKey;
      pc.amount = contribution.amount;
      pc.addedDate = now;
      pc.viewingID = contribution.viewingId;
      pc.category = contribution.category;
    }
  } completion:WriteToDataControllerCompletion(completion)];
}

+ (void)removePendingContributionForPublisherID:(NSString *)publisherID viewingID:(NSString *)viewingID addedDate:(UInt64)addedDate completion:(BATLedgerDatabaseWriteCompletion)completion
{
  // Early guard to check if object exists.
  // The check happens on main thread, while the deletion is done in background.
  const auto pendingContribution = [self getPendingContributonWithPublisherID:publisherID
                                                                    viewingID:viewingID
                                                                    addedDate:addedDate
                                                                      context:DataController.viewContext];
  if (!pendingContribution) {
    if (completion) { completion(NO); }
    return;
  }

  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    const auto pendingContribution = [self getPendingContributonWithPublisherID:publisherID
                                                                      viewingID:viewingID
                                                                      addedDate:addedDate
                                                                        context:context];
    [context deleteObject:pendingContribution];
  } completion:WriteToDataControllerCompletion(completion)];
}

+ (void)removeAllPendingContributions:(BATLedgerDatabaseWriteCompletion)completion
{
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    const auto fetchRequest = PendingContribution.fetchRequest;
    fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(PendingContribution.class)
                                      inManagedObjectContext:context];

    NSError *error;
    const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
    if (error) {
      NSLog(@"%@", error);
      completion(NO);
      return;
    }

    for (PendingContribution *pc in fetchedObjects) {
      [context deleteObject:pc];
    }
  } completion:WriteToDataControllerCompletion(completion)];
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

#pragma mark - Publisher List / Info

+ (nullable BATServerPublisherInfo *)serverPublisherInfoWithPublisherID:(NSString *)publisherID
{
  const auto serverInfo = [self getServerPublisherInfoWithPublisherID:publisherID context:DataController.viewContext];
  if (!serverInfo) {
    return nil;
  }
  
  const auto info = [[BATServerPublisherInfo alloc] init];
  info.publisherKey = publisherID;
  info.status = static_cast<BATPublisherStatus>(serverInfo.status);
  info.excluded = serverInfo.excluded;
  info.address = serverInfo.address;
  return info;
}

+ (void)insertOrUpdateServerPublisherList:(NSArray<BATServerPublisherInfo *> *)list context:(NSManagedObjectContext *)context
{
  for (BATServerPublisherInfo *info in list) {
    auto pi = [self getServerPublisherInfoWithPublisherID:info.publisherKey context:context] ?: [[ServerPublisherInfo alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(ServerPublisherInfo.class) inManagedObjectContext:context] insertIntoManagedObjectContext:context];
    pi.publisherID = info.publisherKey;
    pi.status = static_cast<int32_t>(info.status);
    pi.excluded = info.excluded;
    pi.address = info.address;
  }
}

+ (void)insertOrUpdateServerPublisherList:(NSArray<BATServerPublisherInfo *> *)list
                               completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    [self insertOrUpdateServerPublisherList:list context:context];
  } completion:WriteToDataControllerCompletion(completion)];
}

+ (void)clearAndInsertList:(NSArray<BATServerPublisherInfo *> *)list
                completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    const auto fetchRequest = ServerPublisherInfo.fetchRequest;
    fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(ServerPublisherInfo.class)
                                      inManagedObjectContext:context];
    NSError *error = nil;
    if ([DataController.shared.container.persistentStoreDescriptions.firstObject.type isEqualToString:NSSQLiteStoreType]) {
      const auto deleteRequest = [[NSBatchDeleteRequest alloc] initWithFetchRequest:fetchRequest];
      [context executeRequest:deleteRequest error:&error];
    } else {
      const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
      for (ServerPublisherInfo *spi in fetchedObjects) {
        [context deleteObject:spi];
      }
    }
    if (error) {
      NSLog(@"%@", error);
    }
    [self insertOrUpdateServerPublisherList:list context:context];
  } completion:WriteToDataControllerCompletion(completion)];
}

#pragma mark - Publisher List / Banner Amounts

+ (nullable NSArray<NSNumber *> *)bannerAmountsForPublisherWithPublisherID:(NSString *)publisherID
{
  const auto context = DataController.viewContext;
  const auto fetchRequest = ServerPublisherAmount.fetchRequest;
  fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(ServerPublisherAmount.class)
                                    inManagedObjectContext:context];
  fetchRequest.predicate = [NSPredicate predicateWithFormat:@"publisherID == %@", publisherID];
  fetchRequest.sortDescriptors = @[ [[NSSortDescriptor alloc] initWithKey:@"amount" ascending:YES] ];
  
  NSError *error;
  const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
  if (error) {
    NSLog(@"%s: %@", __PRETTY_FUNCTION__, error);
  }
  
  const auto amounts = [[NSMutableArray<NSNumber *> alloc] init];
  for (ServerPublisherAmount *amount in fetchedObjects) {
    [amounts addObject:@(amount.amount)];
  }
  
  if (amounts.count == 0) {
    return nil;
  }
  return amounts;
}

+ (void)insertOrUpdateBannerAmountsForServerPublisherInfo:(BATServerPublisherInfo *)info context:(NSManagedObjectContext *)context
{
  for (NSNumber *amountObj in info.banner.amounts) {
    double amount = [amountObj doubleValue];
    auto spa = [self getServerPublisherAmountWithPublisherID:info.publisherKey amount:amount context:context] ?: [[ServerPublisherAmount alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(ServerPublisherAmount.class) inManagedObjectContext:context] insertIntoManagedObjectContext:context];
    spa.serverPublisherInfo = [self getServerPublisherInfoWithPublisherID:info.publisherKey context:context];
    spa.amount = amount;
    spa.publisherID = info.publisherKey;
  }
}

+ (void)insertOrUpdateBannerAmountsForServerPublisherInfo:(BATServerPublisherInfo *)info
                                               completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  if (!info.banner) {
    completion(NO);
    return;
  }
  
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    [self insertOrUpdateBannerAmountsForServerPublisherInfo:info context:context];
  } completion:WriteToDataControllerCompletion(completion)];
}

#pragma mark - Publisher List / Banner

+ (nullable BATPublisherBanner *)bannerForPublisherID:(NSString *)publisherID
{
  const auto spb = [self getServerPublisherBannerWithPublisherID:publisherID context:DataController.viewContext];
  if (!spb) {
    return nil;
  }
  const auto banner = [[BATPublisherBanner alloc] init];
  banner.publisherKey = publisherID;
  banner.title = spb.title;
  banner.desc = spb.desc;
  banner.background = spb.background;
  banner.logo = spb.logo;
  banner.links = [self publisherLinksWithPublisherID:publisherID];
  banner.amounts = [self bannerAmountsForPublisherWithPublisherID:publisherID];
  return banner;
}

+ (void)insertOrUpdateBannerForServerPublisherInfo:(BATServerPublisherInfo *)info
                                        completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  if (!info.banner) {
    completion(NO);
    return;
  }
  
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    auto spb = [self getServerPublisherBannerWithPublisherID:info.publisherKey context:context] ?: [[ServerPublisherBanner alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(ServerPublisherBanner.class) inManagedObjectContext:context] insertIntoManagedObjectContext:context];
    spb.serverPublisherInfo = [self getServerPublisherInfoWithPublisherID:info.publisherKey context:context];
    spb.publisherID = info.publisherKey;
    spb.title = info.banner.title;
    spb.desc = info.banner.desc;
    spb.background = info.banner.background;
    spb.logo = info.banner.logo;
    
    // This is the same as core, they insert banner, amounts and links all at the same time
    [self insertOrUpdateBannerAmountsForServerPublisherInfo:info context:context];
    [self insertOrUpdateLinksForServerPublisherInfo:info context:context];
  } completion:WriteToDataControllerCompletion(completion)];
}

#pragma mark - Publisher List / Link

+ (nullable NSDictionary<NSString *, NSString *> *)publisherLinksWithPublisherID:(NSString *)publisherID
{
  const auto context = DataController.viewContext;
  const auto fetchRequest = ServerPublisherLink.fetchRequest;
  fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(ServerPublisherLink.class)
                                    inManagedObjectContext:context];
  fetchRequest.predicate = [NSPredicate predicateWithFormat:@"publisherID == %@", publisherID];
  
  NSError *error;
  const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
  if (error) {
    NSLog(@"%s: %@", __PRETTY_FUNCTION__, error);
  }
  
  const auto links = [[NSMutableDictionary<NSString *, NSString *> alloc] init];
  for (ServerPublisherLink *link in fetchedObjects) {
    links[link.provider] = link.link;
  }
  if (links.count == 0) {
    return nil;
  }
  return links;
}

+ (void)insertOrUpdateLinksForServerPublisherInfo:(BATServerPublisherInfo *)info context:(NSManagedObjectContext *)context
{
  for (NSString *provider in info.banner.links) {
    auto spl = [self getServerPublisherLinkWithPublisherID:info.publisherKey provider:provider context:context] ?: [[ServerPublisherLink alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(ServerPublisherLink.class) inManagedObjectContext:context] insertIntoManagedObjectContext:context];
    spl.serverPublisherInfo = [self getServerPublisherInfoWithPublisherID:info.publisherKey context:context];
    spl.publisherID = info.publisherKey;
    spl.provider = provider;
    spl.link = info.banner.links[provider];
  }
}

+ (void)insertOrUpdateLinksForServerPublisherInfo:(BATServerPublisherInfo *)info
                                       completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  if (!info.banner) {
    completion(NO);
    return;
  }
  
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    [self insertOrUpdateLinksForServerPublisherInfo:info context:context];
  } completion:WriteToDataControllerCompletion(completion)];
}

@end
