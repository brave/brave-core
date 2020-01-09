/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "BATLedgerDatabase.h"

#import "DataController.h"
#import "CoreDataModels.h"
#import "bat/ledger/global_constants.h"
#import "RewardsLogStream.h"

#define BLOG(__severity) RewardsLogStream(__FILE__, __LINE__, __severity).stream()

// TO BE REMOVED:
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

+ (nullable NSString *)migrateCoreDataToSQLTransaction
{
  const auto bundlePath = [[NSBundle bundleForClass:BATLedgerDatabase.class] pathForResource:@"migrate" ofType:@"sql"];
  NSError *error = nil;
  const auto migrationScript = [NSString stringWithContentsOfFile:bundlePath encoding:NSUTF8StringEncoding error:&error];
  if (error) {
    BLOG(ledger::LogLevel::LOG_ERROR) << "Failed to load migration script from path: " << bundlePath.UTF8String << std::endl;
    return nil;
  }
  
  const auto statements = [[NSMutableArray alloc] init];
  
  // activity_info
  const auto activityInfoInsert =
    @"INSERT INTO \"activity_info\" "
     "(publisher_id, duration, visits, score, percent, weight, reconcile_stamp) VALUES ("
     "%@,"   // publisher_id LONGVARCHAR NOT NULL
     "%lld," // duration INTEGER DEFAULT 0 NOT NULL,
     "%d,"   // visits INTEGER DEFAULT 0 NOT NULL,
     "%f,"   // score DOUBLE DEFAULT 0 NOT NULL
     "%d,"   // percent INTEGER DEFAULT 0 NOT NULL
     "%f,"   // weight DOUBLE DEFAULT 0 NOT NULL,
     "%lld"  // reconcile_stamp INTEGER DEFAULT 0 NOT NULL
     ");";
  
  [statements addObjectsFromArray:
   MapFetchedObjectsToInsertsForClass(ActivityInfo.class, ^(ActivityInfo *info){
    return [NSString stringWithFormat:activityInfoInsert,
            SQLString(info.publisherID), info.duration, info.visits, info.score,
            info.percent, info.weight, info.reconcileStamp];
  })];
  
  // contribution_info
  const auto contributionInfoInsert =
    @"INSERT INTO \"contribution_info\" "
     "(publisher_id, probi, date, type, month, year) VALUES ("
     "%@,"   // publisher_id LONGVARCHAR
     "%@,"   // probi TEXT "0"  NOT NULL
     "%lld," // date INTEGER NOT NULL
     "%d,"   // type INTEGER NOT NULL
     "%d,"   // month INTEGER NOT NULL
     "%d"    // year INTEGER NOT NULL
     ");";
  
  [statements addObjectsFromArray:
   MapFetchedObjectsToInsertsForClass(ContributionInfo.class, ^(ContributionInfo *info){
    return [NSString stringWithFormat:contributionInfoInsert,
            SQLString(info.publisherID), SQLString(info.probi), info.date,
            info.type, info.month, info.year];
  })];
  
  // contribution_queue
  const auto contributionQueueInsert =
    @"INSERT INTO \"contribution_queue\" "
     "(contribution_queue_id, type, amount, partial) VALUES ("
     "%lld," // contribution_queue_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL
     "%d,"   // type INTEGER NOT NULL
     "%f,"   // amount DOUBLE NOT NULL
     "%d"    // partial INTEGER NOT NULL DEFAULT 0
     ");";
  
  __block int64_t contributionQueueMaxID = 0;
  [statements addObjectsFromArray:
   MapFetchedObjectsToInsertsForClass(ContributionQueue.class, ^(ContributionQueue *obj){
    contributionQueueMaxID = MAX(obj.id, contributionQueueMaxID);
    return [NSString stringWithFormat:contributionQueueInsert,
            obj.id, obj.type, obj.amount, obj.partial];
  })];
  [statements addObject:
   [NSString stringWithFormat:@"UPDATE SQLITE_SEQUENCE SET seq = %lld WHERE name = 'contribution_queue';", contributionQueueMaxID]
   ];
  
  // contribution_queue_publishers
  const auto contributionQueuePublisherInsert =
   @"INSERT INTO \"contribution_queue_publishers\" "
    "(contribution_queue_id, publisher_key, amount_percent) VALUES ("
    "%lld," // contribution_queue_id INTEGER NOT NULL
    "%@,"   // publisher_key TEXT NOT NULL
    "%f"    // amount_percent DOUBLE NOT NULL
    ");";
  [statements addObjectsFromArray:
   MapFetchedObjectsToInsertsForClass(ContributionPublisher.class, ^(ContributionPublisher *obj){
    return [NSString stringWithFormat:contributionQueuePublisherInsert,
            obj.queue.id, SQLString(obj.publisherKey), obj.amountPercent];
  })];
  
  // media_publisher_info
  const auto mediaPublisherInfoInsert =
    @"INSERT INTO \"media_publisher_info\" "
     "(media_key, publisher_id) VALUES ("
     "%@," // media_key TEXT NOT NULL PRIMARY KEY UNIQUE
     "%@"  // publisher_id LONGVARCHAR NOT NULL
     ");";
  [statements addObjectsFromArray:
   MapFetchedObjectsToInsertsForClass(MediaPublisherInfo.class, ^(MediaPublisherInfo *obj){
    return [NSString stringWithFormat:mediaPublisherInfoInsert,
            SQLString(obj.mediaKey), SQLString(obj.publisherID)];
  })];
  
  // pending_contribution
  const auto pendingContributionInsert =
    @"INSERT INTO \"pending_contribution\" "
     "(publisher_id, amount, added_date, viewing_id, type) VALUES ("
     "%@,"   // publisher_id LONGVARCHAR NOT NULL
     "%f,"   // amount DOUBLE DEFAULT 0 NOT NULL
     "%lld," // added_date INTEGER DEFAULT 0 NOT NULL
     "%@,"   // viewing_id LONGVARCHAR NOT NULL
     "%d"    // type INTEGER NOT NULL
     ");";
  [statements addObjectsFromArray:
   MapFetchedObjectsToInsertsForClass(PendingContribution.class, ^(PendingContribution *obj){
    return [NSString stringWithFormat:pendingContributionInsert,
            SQLString(obj.publisherID), obj.amount, obj.addedDate,
            SQLString(obj.viewingID), obj.type];
  })];
  
  // promotion
  const auto promotionInsert =
    @"INSERT INTO \"promotion\" "
     "(promotion_id, version, type, public_keys, suggestions, "
     "approximate_value, status, expires_at) VALUES ("
     "%@,"  // promotion_id TEXT NOT NULL
     "%d,"  // version INTEGER NOT NULL
     "%d,"  // type INTEGER NOT NULL
     "%@,"  // public_keys TEXT NOT NULL
     "%d,"  // suggestions INTEGER NOT NULL DEFAULT 0
     "%f,"  // approximate_value DOUBLE NOT NULL DEFAULT 0
     "%d,"  // status INTEGER NOT NULL DEFAULT 0
     "%lld" // expires_at TIMESTAMP NOT NULL
     ");";
  [statements addObjectsFromArray:
   MapFetchedObjectsToInsertsForClass(Promotion.class, ^(Promotion *obj){
    return [NSString stringWithFormat:promotionInsert,
            SQLString(obj.promotionID), obj.version, obj.type, SQLString(obj.publicKeys),
            obj.suggestions, obj.approximateValue, obj.status,
            static_cast<int64_t>(obj.expiryDate.timeIntervalSince1970)];
  })];
  
  // promotion_creds
  const auto promotionCredsInsert =
    @"INSERT INTO \"promotion_creds\" "
     "(promotion_id, tokens, blinded_creds, signed_creds, public_key, "
     "batch_proof, claim_id) VALUES ("
     "%@,"  // promotion_id TEXT UNIQUE NOT NULL
     "%@,"  // tokens TEXT NOT NULL
     "%@,"  // blinded_creds TEXT NOT NULL
     "%@,"  // signed_creds TEXT
     "%@,"  // public_key TEXT
     "%@,"  // batch_proof TEXT
     "%@"   // claim_id TEXT
     ");";
  [statements addObjectsFromArray:
   MapFetchedObjectsToInsertsForClass(PromotionCredentials.class, ^(PromotionCredentials *obj){
    return [NSString stringWithFormat:promotionCredsInsert,
            SQLString(obj.promotionID), SQLString(obj.tokens), SQLString(obj.blindedCredentials),
            SQLNullableString(obj.signedCredentials), SQLNullableString(obj.publicKey),
            SQLNullableString(obj.batchProof), SQLNullableString(obj.claimID)];
  })];
  
  // publisher_info
  const auto publisherInfoInsert =
    @"INSERT INTO \"publisher_info\" "
     "(publisher_id, excluded, name, favIcon, url, provider) VALUES ("
     "%@,"  // publisher_id LONGVARCHAR PRIMARY KEY NOT NULL UNIQUE
     "%d,"  // excluded INTEGER DEFAULT 0 NOT NULL
     "%@,"  // name TEXT NOT NULL
     "%@,"  // favIcon TEXT NOT NULL
     "%@,"  // url TEXT NOT NULL
     "%@"   // provider TEXT NOT NULL
     ");";
  [statements addObjectsFromArray:
   MapFetchedObjectsToInsertsForClass(PublisherInfo.class, ^(PublisherInfo *obj){
    return [NSString stringWithFormat:publisherInfoInsert,
            SQLString(obj.publisherID), obj.excluded, SQLString(obj.name),
            SQLString(obj.faviconURL), SQLString(obj.url), SQLString(obj.provider)];
  })];
  
  // recurring_donation
  const auto recurringDonationInsert =
    @"INSERT INTO \"recurring_donation\" "
     "(publisher_id, amount, added_date) VALUES ("
     "%@,"  // publisher_id LONGVARCHAR NOT NULL PRIMARY KEY UNIQUE
     "%f,"  // amount DOUBLE DEFAULT 0 NOT NULL
     "%lld" // added_date INTEGER DEFAULT 0 NOT NULL
     ");";
  [statements addObjectsFromArray:
   MapFetchedObjectsToInsertsForClass(RecurringDonation.class, ^(RecurringDonation *obj){
    return [NSString stringWithFormat:recurringDonationInsert,
            SQLString(obj.publisherID), obj.amount, obj.addedDate];
  })];
  
  // unblinded_tokens
  const auto unblindedTokenInsert =
    @"INSERT INTO \"unblinded_tokens\" "
     "(token_id, token_value, public_key, value, url, promotion_id) VALUES ("
     "%lld," // token_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL
     "%@,"   // token_value TEXT
     "%@,"   // public_key TEXT
     "%f,"   // value DOUBLE NOT NULL DEFAULT 0
     "%@"    // promotion_id TEXT
     ");";
  __block int64_t unblindedTokenMaxID = 0;
  [statements addObjectsFromArray:
   MapFetchedObjectsToInsertsForClass(UnblindedToken.class, ^(UnblindedToken *obj){
    unblindedTokenMaxID = MAX(obj.tokenID, unblindedTokenMaxID);
    return [NSString stringWithFormat:unblindedTokenInsert,
            obj.tokenID, SQLNullableString(obj.tokenValue),
            SQLNullableString(obj.publicKey), obj.value,
            SQLNullableString(obj.promotionID)];
  })];
  [statements addObject:
   [NSString stringWithFormat:@"UPDATE SQLITE_SEQUENCE SET seq = %lld WHERE name = 'unblinded_tokens';", unblindedTokenMaxID]
   ];
  
  return [migrationScript stringByReplacingOccurrencesOfString:@"# {statements}" withString:[statements componentsJoinedByString:@"\n"]];
}

NS_INLINE NSString *SQLNullableString(NSString * _Nullable value) {
  return (value == nil ? @"NULL" : SQLString(value));
}

NS_INLINE NSString *SQLString(NSString * _Nonnull value) {
  // Have to make sure to escape any apostrophies
  return [NSString stringWithFormat:@"'%@'",
          [value stringByReplacingOccurrencesOfString:@"'" withString:@"''"]];
}

static NSArray<NSString *> *
MapFetchedObjectsToInsertsForClass(Class clazz,
                                   NSString * (NS_NOESCAPE ^block)(__kindof NSManagedObject* obj))
{
  const auto context = DataController.viewContext;
  const auto fetchRequest = [clazz fetchRequest];
  fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(clazz)
                                    inManagedObjectContext:context];
  NSError *error;
  const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
  if (error) {
    return @[];
  }
  const auto statements = [[NSMutableArray<NSString *> alloc] init];
  [fetchedObjects enumerateObjectsUsingBlock:^(NSManagedObject *_Nonnull obj, NSUInteger idx, BOOL * _Nonnull stop) {
    if (![obj isKindOfClass:clazz]) { return; }
    [statements addObject:block(obj)];
  }];
  return statements;
}

// TO BE REMOVED:

+ (nullable __kindof NSManagedObject *)firstOfClass:(Class)clazz
                                         predicates:(NSArray<NSPredicate *> *)predicates
                                    sortDescriptors:(NSArray<NSSortDescriptor *> *)sortDescriptors
                                            context:(NSManagedObjectContext *)context
{
  const auto fetchRequest = [clazz fetchRequest];
  fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(clazz)
                                    inManagedObjectContext:context];
  fetchRequest.fetchLimit = 1;
  fetchRequest.sortDescriptors = sortDescriptors;
  fetchRequest.predicate = [NSCompoundPredicate andPredicateWithSubpredicates:predicates];
  
  NSError *error;
  const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
  if (error) {
    BLOG(ledger::LogLevel::LOG_ERROR) << "Failed CoreData fetch request: " << error.debugDescription.UTF8String << std::endl;
  }
  
  return fetchedObjects.firstObject;
}

+ (nullable __kindof NSManagedObject *)firstOfClass:(Class)clazz
                                    withPublisherID:(NSString *)publisherID
                                additionalPredicate:(nullable NSPredicate *)additionalPredicate
                                            context:(NSManagedObjectContext *)context
{
  const auto predicates = [[NSMutableArray<NSPredicate *> alloc] init];
  [predicates addObject:[NSPredicate predicateWithFormat:@"publisherID == %@", publisherID]];

  if (additionalPredicate) {
    [predicates addObject:additionalPredicate];
  }

  return [self firstOfClass:clazz predicates:predicates sortDescriptors:@[] context:context];
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
    BLOG(ledger::LogLevel::LOG_ERROR) << "Failed CoreData fetch request: " << error.debugDescription.UTF8String << std::endl;
  }

  return fetchedObjects.firstObject;
}

+ (nullable ServerPublisherInfo *)getServerPublisherInfoWithPublisherID:(NSString *)publisherID context:(NSManagedObjectContext *)context
{
  return [self firstOfClass:ServerPublisherInfo.class withPublisherID:publisherID additionalPredicate:nil context:context];
}

+ (nullable ServerPublisherBanner *)getServerPublisherBannerWithPublisherID:(NSString *)publisherID context:(NSManagedObjectContext *)context
{
  return [self firstOfClass:ServerPublisherBanner.class withPublisherID:publisherID additionalPredicate:nil context:context];
}

#pragma mark - Publisher Info

+ (BATPublisherInfo *)publisherInfoWithPublisherID:(NSString *)publisherID
{
  auto databaseInfo = [self getPublisherInfoWithID:publisherID context:DataController.viewContext];
  if (!databaseInfo) {
    return nil;
  }
  auto statusInfo = [self getServerPublisherInfoWithPublisherID:publisherID context:DataController.viewContext];
  auto info = [[BATPublisherInfo alloc] init];
  info.id = databaseInfo.publisherID;
  info.name = databaseInfo.name;
  info.url = databaseInfo.url;
  info.faviconUrl = databaseInfo.faviconURL;
  info.provider = databaseInfo.provider;
  info.status = static_cast<BATPublisherStatus>(statusInfo.status);
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
    BLOG(ledger::LogLevel::LOG_ERROR) << "Failed CoreData fetch request: " << error.debugDescription.UTF8String << std::endl;
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
      BLOG(ledger::LogLevel::LOG_ERROR) << "Failed CoreData fetch request: " << error.debugDescription.UTF8String << std::endl;
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
    BLOG(ledger::LogLevel::LOG_ERROR) << "Failed CoreData fetch request: " << error.debugDescription.UTF8String << std::endl;
  }
  return count;
}

#pragma mark - Contribution Info

+ (void)insertContributionInfo:(NSString *)probi month:(const BATActivityMonth)month year:(const int)year date:(const uint32_t)date publisherKey:(NSString *)publisherKey type:(BATRewardsType)type completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    auto ci = [[ContributionInfo alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(ContributionInfo.class) inManagedObjectContext:context]
                        insertIntoManagedObjectContext:context];
    ci.probi = probi;
    ci.month = month;
    ci.year = year;
    ci.date = date;
    ci.publisherID = publisherKey;
    ci.type = type;
    ci.publisher = [self getPublisherInfoWithID:publisherKey context:context];
  } completion:WriteToDataControllerCompletion(completion)];
}

+ (NSArray<BATPublisherInfo *> *)oneTimeTipsPublishersForMonth:(BATActivityMonth)month year:(int)year
{
  const auto context = DataController.viewContext;
  const auto fetchRequest = ContributionInfo.fetchRequest;
  fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(ContributionInfo.class)
                                    inManagedObjectContext:context];
  fetchRequest.predicate = [NSPredicate predicateWithFormat:@"month = %d AND year = %d AND type = %d",
                            month, year, BATRewardsTypeOneTimeTip];

  NSError *error;
  const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
  if (error) {
    BLOG(ledger::LogLevel::LOG_ERROR) << "Failed CoreData fetch request: " << error.debugDescription.UTF8String << std::endl;
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
    info.status = static_cast<BATPublisherStatus>([self getServerPublisherInfoWithPublisherID:ci.publisherID context:context].status);
    info.provider = ci.publisher.provider;
    [publishers addObject:info];
  }
  return publishers;
}

#pragma mark - Contribution Queue

+ (nullable ContributionQueue *)getContributionQueueWithID:(int64_t)queueID context:(NSManagedObjectContext *)context
{
  const auto predicate = [NSPredicate predicateWithFormat:@"id == %ld", queueID];
  return [self firstOfClass:ContributionQueue.class predicates:@[predicate] sortDescriptors:@[] context:context];
}

+ (nullable ContributionPublisher *)getContributionPublisherWithQueueID:(int64_t)queueID context:(NSManagedObjectContext *)context
{
  const auto predicate = [NSPredicate predicateWithFormat:@"queue.id == %ld", queueID];
  return [self firstOfClass:ContributionPublisher.class predicates:@[predicate] sortDescriptors:@[] context:context];
}

+ (void)insertOrUpdateContributionQueue:(BATContributionQueue *)queue
                             completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    const auto cq = [self getContributionQueueWithID:queue.id context:context] ?:
    [[ContributionQueue alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(ContributionQueue.class) inManagedObjectContext:context]
           insertIntoManagedObjectContext:context];
    cq.id = queue.id;
    cq.type = static_cast<int64_t>(queue.type);
    cq.amount = queue.amount;
    cq.partial = queue.partial;
    [self insertContributionPublishersIntoQueue:cq publishers:queue.publishers context:context];
  } completion:WriteToDataControllerCompletion(completion)];
}

+ (BATContributionQueue *)firstQueue
{
  auto sort = [[NSSortDescriptor alloc] initWithKey:@"id" ascending:YES];
  ContributionQueue *cq = [self firstOfClass:ContributionQueue.class
                                  predicates:@[]
                             sortDescriptors:@[sort]
                                     context:DataController.viewContext];
  if (!cq) {
    return nil;
  }
  
  BATContributionQueue *queue = [[BATContributionQueue alloc] init];
  queue.id = cq.id;
  queue.partial = cq.partial;
  queue.amount = cq.amount;
  queue.type = static_cast<BATRewardsType>(cq.type);
  queue.publishers = ^NSArray *{
    NSMutableArray *pubs = [[NSMutableArray alloc] init];
    for (ContributionPublisher *pub in cq.publishers.allObjects) {
      auto queuePublisher = [[BATContributionQueuePublisher alloc] init];
      queuePublisher.amountPercent = pub.amountPercent;
      queuePublisher.publisherKey = pub.publisherKey;
      [pubs addObject:queuePublisher];
    }
    return [pubs copy];
  }();
  return queue;
}

+ (void)deleteQueueWithID:(int64_t)queueID
               completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    const auto cq = [self getContributionQueueWithID:queueID context:context];
    if (!cq) {
      dispatch_async(dispatch_get_main_queue(), ^{
        completion(NO);
      });
      return;
    }
    [context deleteObject:cq];
  } completion:WriteToDataControllerCompletion(completion)];
}

+ (void)insertContributionPublishersIntoQueue:(ContributionQueue *)queue
                                   publishers:(NSArray<BATContributionQueuePublisher *> *)publishers
                                      context:(NSManagedObjectContext *)context
{
  for (BATContributionQueuePublisher *publisher in publishers) {
    const auto cp = [[ContributionPublisher alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(ContributionPublisher.class) inManagedObjectContext:context]
               insertIntoManagedObjectContext:context];
    cp.queue = queue;
    cp.publisherKey = publisher.publisherKey;
    cp.amountPercent = publisher.amountPercent;
  }
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
    BLOG(ledger::LogLevel::LOG_ERROR) << "Failed CoreData fetch request: " << error.debugDescription.UTF8String << std::endl;
  }

  const auto publishers = [[NSMutableArray<BATPublisherInfo *> alloc] init];
  for (ActivityInfo *activity in fetchedObjects) {
    const auto status = static_cast<BATPublisherStatus>([self getServerPublisherInfoWithPublisherID:activity.publisherID context:context].status);
    if (!filter.nonVerified && status == BATPublisherStatusNotVerified) {
      continue;
    }
    auto info = [[BATPublisherInfo alloc] init];
    info.id = activity.publisherID;
    info.duration = activity.duration;
    info.score = activity.score;
    info.percent = activity.percent;
    info.weight = activity.weight;
    info.status = status;
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
      BLOG(ledger::LogLevel::LOG_ERROR) << "Failed CoreData fetch request: " << error.debugDescription.UTF8String << std::endl;
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
    BLOG(ledger::LogLevel::LOG_ERROR) << "Failed CoreData fetch request: " << error.debugDescription.UTF8String << std::endl;
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
    info.status = static_cast<BATPublisherStatus>([self getServerPublisherInfoWithPublisherID:rd.publisherID context:context].status);
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
    BLOG(ledger::LogLevel::LOG_ERROR) << "Failed CoreData fetch request: " << error.debugDescription.UTF8String << std::endl;
  }

  const auto publishers = [[NSMutableArray<BATPendingContributionInfo *> alloc] init];
  for (PendingContribution *pc in fetchedObjects) {
    auto info = [[BATPendingContributionInfo alloc] init];
    info.publisherKey = pc.publisherID;
    info.name = pc.publisher.name;
    info.url = pc.publisher.url;
    info.faviconUrl = pc.publisher.faviconURL;
    info.status = static_cast<BATPublisherStatus>([self getServerPublisherInfoWithPublisherID:pc.publisherID context:context].status);
    info.provider = pc.publisher.provider;
    info.amount = pc.amount;
    info.addedDate = pc.addedDate;
    info.viewingId = pc.viewingID;
    info.type = static_cast<BATRewardsType>(pc.type);
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
      pc.type = contribution.type;
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
      BLOG(ledger::LogLevel::LOG_ERROR) << "Failed CoreData fetch request: " << error.debugDescription.UTF8String << std::endl;
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
    BLOG(ledger::LogLevel::LOG_ERROR) << "Failed CoreData fetch request: " << error.debugDescription.UTF8String << std::endl;
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
  
  const auto banner = [[BATPublisherBanner alloc] init];
  banner.publisherKey = publisherID;
  if (serverInfo.banner) {
    banner.title = serverInfo.banner.title;
    banner.desc = serverInfo.banner.desc;
    banner.background = serverInfo.banner.background;
    banner.logo = serverInfo.banner.logo;
  }
  banner.links = ^NSDictionary *{
    NSMutableDictionary *links = [[NSMutableDictionary alloc] init];
    for (ServerPublisherLink *link in serverInfo.links) {
      links[link.provider] = link.link;
    }
    return [links copy];
  }();
  banner.amounts = ^NSArray *{
    NSMutableArray *amounts = [[NSMutableArray alloc] init];
    for (ServerPublisherAmount *amount in serverInfo.amounts) {
      [amounts addObject:@(amount.amount)];
    }
    return [amounts copy];
  }();
  info.banner = banner;
  
  return info;
}

+ (void)insertOrUpdateServerPublisherList:(NSArray<BATServerPublisherInfo *> *)list context:(NSManagedObjectContext *)context
{
  for (BATServerPublisherInfo *info in list) {
    auto pi = [[ServerPublisherInfo alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(ServerPublisherInfo.class) inManagedObjectContext:context] insertIntoManagedObjectContext:context];
    pi.publisherID = info.publisherKey;
    pi.status = static_cast<int32_t>(info.status);
    pi.excluded = info.excluded;
    pi.address = info.address;
    [self insertOrUpdateBannerForServerPublisherInfo:pi banner:info.banner context:context];
  }
}

+ (void)clearAndInsertList:(NSArray<BATServerPublisherInfo *> *)list
                completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    const auto batchSize = 1000.0;
    const auto numberOfSplits = (NSInteger)ceil(list.count / batchSize);
    NSMutableArray *split = [[NSMutableArray alloc] init];
    for (NSUInteger i = 0; i < numberOfSplits; i++) {
      auto index = i * batchSize;
      NSArray* sub = [[list subarrayWithRange:NSMakeRange(index, MIN(batchSize, list.count - index))] copy];
      [split addObject:sub];
    }
    BLOG(ledger::LogLevel::LOG_INFO) << "CoreData: Splitting " << list.count << " records into " << numberOfSplits << " batches" << std::endl;
    
    const auto context = [DataController newBackgroundContext];
    
    NSDate *start = [NSDate date];
    BLOG(ledger::LogLevel::LOG_INFO) << "CoreData: Deleting publisher list" << std::endl;
    [context performBlockAndWait:^{
      const auto fetchRequest = ServerPublisherInfo.fetchRequest;
      fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(ServerPublisherInfo.class)
                                        inManagedObjectContext:context];
      NSError *error = nil;
      const auto deleteRequest = [[NSBatchDeleteRequest alloc] initWithFetchRequest:fetchRequest];
      [context executeRequest:deleteRequest error:&error];
      
      if (context.hasChanges) {
        [context save:nil];
      }
    }];
    BLOG(ledger::LogLevel::LOG_INFO) << "CoreData: Deleted publisher list in " << [[NSDate date] timeIntervalSinceDate:start] << " seconds" << std::endl;
    const auto operationQueue = [[NSOperationQueue alloc] init];
    // 2 concurrent ops operate better than 4 or 6 when testing
    operationQueue.maxConcurrentOperationCount = MIN(2, [[NSProcessInfo processInfo] activeProcessorCount]);
    operationQueue.name = @"Publisher List Insertions";
    BLOG(ledger::LogLevel::LOG_INFO) << "CoreData: Inserting " << list.count << " publisher info objects" << std::endl;
    for (NSArray *section in split) {
      [operationQueue addOperationWithBlock:^{
        const auto newContext = [DataController newBackgroundContext];
        [newContext performBlockAndWait:^{
          [self insertOrUpdateServerPublisherList:section context:newContext];
          if (newContext.hasChanges) {
            NSError *error;
            if (![newContext save:&error]) {
              BLOG(ledger::LogLevel::LOG_ERROR) << "CoreData: Save error: " << error.debugDescription.UTF8String << std::endl;
            }
          }
        }];
      }];
    }
    [operationQueue waitUntilAllOperationsAreFinished];
    NSTimeInterval duration = [[NSDate date] timeIntervalSinceDate:start];
    BLOG(ledger::LogLevel::LOG_INFO) << "CoreData: Completed publisher list task. Took total of " << duration << " seconds" << std::endl;
    dispatch_async(dispatch_get_main_queue(), ^{
      completion(true);
    });
  });
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

+ (void)insertOrUpdateBannerForServerPublisherInfo:(ServerPublisherInfo *)info
                                            banner:(BATPublisherBanner *)banner
                                           context:(NSManagedObjectContext *)context
{
  if (!banner) {
    return;
  }
  
  auto spb = [[ServerPublisherBanner alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(ServerPublisherBanner.class) inManagedObjectContext:context] insertIntoManagedObjectContext:context];
  if (info.managedObjectContext == context) {
    spb.serverPublisherInfo = info;
  } else {
    spb.serverPublisherInfo = [context objectWithID:info.objectID];
  }
  spb.publisherID = info.publisherID;
  spb.title = banner.title;
  spb.desc = banner.desc;
  spb.background = banner.background;
  spb.logo = banner.logo;
  
  // This is the same as core, they insert banner, amounts and links all at the same time
  [self insertOrUpdateBannerAmountsForServerPublisherInfo:info banner:banner context:context];
  [self insertOrUpdateLinksForServerPublisherInfo:info banner:banner context:context];
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
    BLOG(ledger::LogLevel::LOG_ERROR) << "Failed CoreData fetch request: " << error.debugDescription.UTF8String << std::endl;
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

+ (void)insertOrUpdateBannerAmountsForServerPublisherInfo:(ServerPublisherInfo *)info
                                                   banner:(BATPublisherBanner *)banner
                                                  context:(NSManagedObjectContext *)context
{
  if (!banner) {
    return;
  }
  
  for (NSNumber *amountObj in banner.amounts) {
    double amount = [amountObj doubleValue];
    auto spa = [[ServerPublisherAmount alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(ServerPublisherAmount.class) inManagedObjectContext:context] insertIntoManagedObjectContext:context];
    if (info.managedObjectContext == context) {
      spa.serverPublisherInfo = info;
    } else {
      spa.serverPublisherInfo = [context objectWithID:info.objectID];
    }
    spa.amount = amount;
    spa.publisherID = info.publisherID;
  }
}

#pragma mark - Publisher List / Banner Links

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
    BLOG(ledger::LogLevel::LOG_ERROR) << "Failed CoreData fetch request: " << error.debugDescription.UTF8String << std::endl;
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

+ (void)insertOrUpdateLinksForServerPublisherInfo:(ServerPublisherInfo *)info
                                           banner:(BATPublisherBanner *)banner
                                          context:(NSManagedObjectContext *)context
{
  if (!banner) {
    return;
  }
  for (NSString *provider in banner.links) {
    if ([(NSString *)banner.links[provider] length] == 0) {
      continue;
    }
    auto spl = [[ServerPublisherLink alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(ServerPublisherLink.class) inManagedObjectContext:context] insertIntoManagedObjectContext:context];
    if (info.managedObjectContext == context) {
      spl.serverPublisherInfo = info;
    } else {
      spl.serverPublisherInfo = [context objectWithID:info.objectID];
    }
    spl.publisherID = info.publisherID;
    spl.provider = provider;
    spl.link = banner.links[provider];
  }
}

#pragma mark - Promotions

+ (nullable Promotion *)getPromotionWithID:(NSString *)promoID context:(NSManagedObjectContext *)context
{
  return [self firstOfClass:Promotion.class
                 predicates:@[[NSPredicate predicateWithFormat:@"promotionID == %@", promoID]]
            sortDescriptors:@[]
                    context:context];
}

+ (nullable PromotionCredentials *)getPromoCredsWithPromotionID:(NSString *)promotionID context:(NSManagedObjectContext *)context
{
  return [self firstOfClass:PromotionCredentials.class
                 predicates:@[[NSPredicate predicateWithFormat:@"promotionID == %@", promotionID]]
            sortDescriptors:@[]
                    context:context];
}

+ (nullable UnblindedToken *)getUnblindedTokenWithID:(UInt64)tokenID context:(NSManagedObjectContext *)context
{
  return [self firstOfClass:UnblindedToken.class
                 predicates:@[[NSPredicate predicateWithFormat:@"tokenID == %lld", tokenID]]
            sortDescriptors:@[]
                    context:context];
}

+ (BATPromotion *)promotiomFromDBPromotion:(Promotion *)dbPromo context:(NSManagedObjectContext *)context
{
  const auto promotion = [[BATPromotion alloc] init];
  promotion.id = dbPromo.promotionID;
  promotion.version = dbPromo.version;
  promotion.type = static_cast<BATPromotionType>(dbPromo.type);
  promotion.publicKeys = dbPromo.publicKeys;
  promotion.suggestions = dbPromo.suggestions;
  promotion.approximateValue = dbPromo.approximateValue;
  promotion.status = static_cast<BATPromotionStatus>(dbPromo.status);
  promotion.expiresAt = [dbPromo.expiryDate timeIntervalSince1970];
  promotion.credentials = ^BATPromotionCreds * _Nullable {
    const auto dbCreds = [self getPromoCredsWithPromotionID:dbPromo.promotionID context:context];
    if (!dbCreds) {
      return nil;
    }
    const auto creds = [[BATPromotionCreds alloc] init];
    creds.claimId = dbCreds.claimID;
    creds.blindedCreds = dbCreds.blindedCredentials;
    creds.signedCreds = dbCreds.signedCredentials;
    creds.publicKey = dbCreds.publicKey;
    creds.batchProof = dbCreds.batchProof;
    creds.tokens = dbCreds.tokens;
    return creds;
  }();
  return promotion;
}

+ (NSArray<BATPromotion *> *)allPromotions
{
  const auto context = DataController.viewContext;
  const auto fetchRequest = [Promotion fetchRequest];
  fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(Promotion.class)
                                    inManagedObjectContext:context];
  NSError *error;
  const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
  if (error) {
    BLOG(ledger::LogLevel::LOG_ERROR) << "Failed CoreData fetch request: " << error.debugDescription.UTF8String << std::endl;
    return @[];
  }
  
  auto promos = [[NSMutableArray<BATPromotion *> alloc] init];
  for (Promotion *dbPromotion in fetchedObjects) {
    [promos addObject:[self promotiomFromDBPromotion:dbPromotion context:context]];
  }
  return [promos copy];
}

+ (void)insertOrUpdatePromotion:(BATPromotion *)promotion
                     completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  [DataController.shared performOnContext:DataController.viewContext task:^(NSManagedObjectContext * _Nonnull context) {
    auto promo = [self getPromotionWithID:promotion.id context:context] ?: [[Promotion alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(Promotion.class) inManagedObjectContext:context]
                                                                                             insertIntoManagedObjectContext:context];
    promo.promotionID = promotion.id;
    promo.version = promotion.version;
    promo.type = static_cast<int64_t>(promotion.type);
    promo.publicKeys = promotion.publicKeys;
    promo.suggestions = promotion.suggestions;
    promo.approximateValue = promotion.approximateValue;
    promo.status = static_cast<int32_t>(promotion.status);
    promo.expiryDate = [NSDate dateWithTimeIntervalSince1970:promotion.expiresAt];
    if (promotion.credentials != nil) {
      auto creds = [self getPromoCredsWithPromotionID:promotion.id context:context] ?: [[PromotionCredentials alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(PromotionCredentials.class) inManagedObjectContext:context] insertIntoManagedObjectContext:context];
      creds.promotionID = promotion.id;
      creds.claimID = promotion.credentials.claimId;
      creds.batchProof = promotion.credentials.batchProof;
      creds.publicKey = promotion.credentials.publicKey;
      creds.signedCredentials = promotion.credentials.signedCreds;
      creds.blindedCredentials = promotion.credentials.blindedCreds;
      creds.tokens = promotion.credentials.tokens;
    }
  } completion:WriteToDataControllerCompletion(completion)];
}

+ (nullable BATPromotion *)promotionWithID:(NSString *)promoID
{
  const auto context = DataController.viewContext;
  const auto dbPromo = [self getPromotionWithID:promoID context:context];
  if (!dbPromo) {
    return nil;
  }
  return [self promotiomFromDBPromotion:dbPromo context:context];
}

+ (void)insertOrUpdateUnblindedToken:(BATUnblindedToken *)unblindedToken
                          completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    auto token = [self getUnblindedTokenWithID:unblindedToken.id context:context] ?: [[UnblindedToken alloc] initWithEntity:[NSEntityDescription entityForName:NSStringFromClass(UnblindedToken.class) inManagedObjectContext:context]
                                                                                                   insertIntoManagedObjectContext:context];
    token.tokenID = unblindedToken.id;
    token.publicKey = unblindedToken.publicKey;
    token.value = unblindedToken.value;
    token.promotionID = unblindedToken.promotionId;
    token.tokenValue = unblindedToken.tokenValue;
  } completion:WriteToDataControllerCompletion(completion)];
}

+ (NSArray<BATUnblindedToken *> *)allUnblindedTokens
{
  const auto context = DataController.viewContext;
  const auto fetchRequest = [UnblindedToken fetchRequest];
  fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(UnblindedToken.class)
                                    inManagedObjectContext:context];
  NSError *error;
  const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
  if (error) {
    BLOG(ledger::LogLevel::LOG_ERROR) << "Failed CoreData fetch request: " << error.debugDescription.UTF8String << std::endl;
    return @[];
  }
  
  auto tokens = [[NSMutableArray<BATUnblindedToken *> alloc] init];
  for (UnblindedToken *dbToken in fetchedObjects) {
    BATUnblindedToken *token = [[BATUnblindedToken alloc] init];
    token.id = dbToken.tokenID;
    token.publicKey = dbToken.publicKey;
    token.value = dbToken.value;
    token.tokenValue = dbToken.tokenValue;
    token.promotionId = dbToken.promotionID;
    const auto promotion = [self getPromotionWithID:dbToken.promotionID context:context];
    if (promotion) {
      token.expiresAt = promotion.expiryDate.timeIntervalSince1970;
    }
    [tokens addObject:token];
  }
  return [tokens copy];
}

+ (void)deleteUnblindedTokens:(NSArray<NSNumber *> *)idList completion:(nullable BATLedgerDatabaseWriteCompletion)completion
{
  [DataController.shared performOnContext:nil task:^(NSManagedObjectContext * _Nonnull context) {
    const auto fetchRequest = UnblindedToken.fetchRequest;
    fetchRequest.entity = [NSEntityDescription entityForName:NSStringFromClass(UnblindedToken.class)
                                      inManagedObjectContext:context];
    fetchRequest.predicate = [NSPredicate predicateWithFormat:@"ANY tokenID IN %@", idList];
    NSError *error;
    const auto fetchedObjects = [context executeFetchRequest:fetchRequest error:&error];
    if (error) {
      BLOG(ledger::LogLevel::LOG_ERROR) << "Failed CoreData fetch request: " << error.debugDescription.UTF8String << std::endl;
      completion(NO);
      return;
    }
    
    for (UnblindedToken *token in fetchedObjects) {
      [context deleteObject:token];
    }
  } completion:WriteToDataControllerCompletion(completion)];
}

@end
