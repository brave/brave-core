/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#import "data_controller.h"
#import "legacy_ledger_database.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BATLedgerDatabase

+ (nullable NSString*)migrateCoreDataToSQLTransaction {
  const auto bundlePath = [[NSBundle bundleForClass:BATLedgerDatabase.class]
      pathForResource:@"migrate"
               ofType:@"sql"];
  NSError* error = nil;
  const auto migrationScript =
      [NSString stringWithContentsOfFile:bundlePath
                                encoding:NSUTF8StringEncoding
                                   error:&error];
  if (error) {
    LOG(ERROR) << "Failed to load migration script from path: "
               << base::SysNSStringToUTF8(bundlePath);
    return nil;
  }

  const auto statements = [[NSMutableArray alloc] init];

  // activity_info
  [statements
      addObjectsFromArray:MapFetchedObjectsToInsertsForClass(
                              ActivityInfo.class, ^(ActivityInfo* info) {
                                return [self activityInfoInsertFor:info];
                              })];

  // contribution_info
  [statements addObjectsFromArray:MapFetchedObjectsToInsertsForClass(
                                      ContributionInfo.class,
                                      ^(ContributionInfo* info) {
                                        return [self
                                            contributionInfoInsertFor:info];
                                      })];

  // contribution_queue
  __block int64_t contributionQueueMaxID = 0;
  [statements addObjectsFromArray:MapFetchedObjectsToInsertsForClass(
                                      ContributionQueue.class,
                                      ^(ContributionQueue* obj) {
                                        contributionQueueMaxID =
                                            MAX(obj.id, contributionQueueMaxID);
                                        return [self
                                            contributionQueueInsertFor:obj];
                                      })];
  if (contributionQueueMaxID > 0) {
    [statements
        addObject:[NSString
                      stringWithFormat:@"UPDATE SQLITE_SEQUENCE SET seq = %lld "
                                       @"WHERE name = 'contribution_queue';",
                                       contributionQueueMaxID]];
  }

  // contribution_queue_publishers
  [statements
      addObjectsFromArray:MapFetchedObjectsToInsertsForClass(
                              ContributionPublisher.class,
                              ^(ContributionPublisher* obj) {
                                return [self
                                    contributionQueuePublisherInsertFor:obj];
                              })];

  // media_publisher_info
  [statements addObjectsFromArray:MapFetchedObjectsToInsertsForClass(
                                      MediaPublisherInfo.class,
                                      ^(MediaPublisherInfo* obj) {
                                        return [self
                                            mediaPublisherInfoInsertFor:obj];
                                      })];

  // pending_contribution
  [statements addObjectsFromArray:MapFetchedObjectsToInsertsForClass(
                                      PendingContribution.class,
                                      ^(PendingContribution* obj) {
                                        return [self
                                            pendingContributionInsertFor:obj];
                                      })];

  // promotion
  [statements addObjectsFromArray:MapFetchedObjectsToInsertsForClass(
                                      Promotion.class, ^(Promotion* obj) {
                                        return [self promotionInsertFor:obj];
                                      })];

  // promotion_creds
  [statements addObjectsFromArray:MapFetchedObjectsToInsertsForClass(
                                      PromotionCredentials.class,
                                      ^(PromotionCredentials* obj) {
                                        return
                                            [self promotionCredsInsertFor:obj];
                                      })];

  // publisher_info
  [statements
      addObjectsFromArray:MapFetchedObjectsToInsertsForClass(
                              PublisherInfo.class, ^(PublisherInfo* obj) {
                                return [self publisherInfoInsertFor:obj];
                              })];

  // recurring_donation
  [statements addObjectsFromArray:MapFetchedObjectsToInsertsForClass(
                                      RecurringDonation.class,
                                      ^(RecurringDonation* obj) {
                                        return [self
                                            recurringDonationInsertFor:obj];
                                      })];

  // unblinded_tokens
  __block int64_t unblindedTokenMaxID = 0;
  [statements
      addObjectsFromArray:MapFetchedObjectsToInsertsForClass(
                              UnblindedToken.class, ^(UnblindedToken* obj) {
                                unblindedTokenMaxID =
                                    MAX(obj.tokenID, unblindedTokenMaxID);
                                return [self unblindedTokenInsertFor:obj];
                              })];
  if (unblindedTokenMaxID > 0) {
    [statements
        addObject:[NSString
                      stringWithFormat:@"UPDATE SQLITE_SEQUENCE SET seq = %lld "
                                       @"WHERE name = 'unblinded_tokens';",
                                       unblindedTokenMaxID]];
  }

  return [migrationScript
      stringByReplacingOccurrencesOfString:@"# {statements}"
                                withString:[statements
                                               componentsJoinedByString:@"\n"]];
}

+ (nullable NSString*)migrateCoreDataBATOnlyToSQLTransaction {
  const auto bundlePath = [[NSBundle bundleForClass:BATLedgerDatabase.class]
      pathForResource:@"migrate"
               ofType:@"sql"];
  NSError* error = nil;
  const auto migrationScript =
      [NSString stringWithContentsOfFile:bundlePath
                                encoding:NSUTF8StringEncoding
                                   error:&error];
  if (error) {
    LOG(ERROR) << "Failed to load migration script from path: "
               << base::SysNSStringToUTF8(bundlePath);
    return nil;
  }

  const auto statements = [[NSMutableArray alloc] init];

  // promotion
  [statements addObjectsFromArray:MapFetchedObjectsToInsertsForClass(
                                      Promotion.class, ^(Promotion* obj) {
                                        return [self promotionInsertFor:obj];
                                      })];

  // promotion_creds
  [statements addObjectsFromArray:MapFetchedObjectsToInsertsForClass(
                                      PromotionCredentials.class,
                                      ^(PromotionCredentials* obj) {
                                        return
                                            [self promotionCredsInsertFor:obj];
                                      })];

  // unblinded_tokens
  __block int64_t unblindedTokenMaxID = 0;
  [statements
      addObjectsFromArray:MapFetchedObjectsToInsertsForClass(
                              UnblindedToken.class, ^(UnblindedToken* obj) {
                                unblindedTokenMaxID =
                                    MAX(obj.tokenID, unblindedTokenMaxID);
                                return [self unblindedTokenInsertFor:obj];
                              })];
  if (unblindedTokenMaxID > 0) {
    [statements
        addObject:[NSString
                      stringWithFormat:@"UPDATE SQLITE_SEQUENCE SET seq = %lld "
                                       @"WHERE name = 'unblinded_tokens';",
                                       unblindedTokenMaxID]];
  }

  return [migrationScript
      stringByReplacingOccurrencesOfString:@"# {statements}"
                                withString:[statements
                                               componentsJoinedByString:@"\n"]];
}

#pragma mark -

+ (NSString*)activityInfoInsertFor:(ActivityInfo*)info {
  const auto activityInfoInsert =
      @"INSERT INTO \"activity_info\" "
       "(publisher_id, duration, visits, score, percent, weight, "
       "reconcile_stamp) VALUES ("
       "%@,"    // publisher_id LONGVARCHAR NOT NULL
       "%lld,"  // duration INTEGER DEFAULT 0 NOT NULL,
       "%d,"    // visits INTEGER DEFAULT 0 NOT NULL,
       "%f,"    // score DOUBLE DEFAULT 0 NOT NULL
       "%d,"    // percent INTEGER DEFAULT 0 NOT NULL
       "%f,"    // weight DOUBLE DEFAULT 0 NOT NULL,
       "%lld"   // reconcile_stamp INTEGER DEFAULT 0 NOT NULL
       ");";
  return [NSString stringWithFormat:activityInfoInsert,
                                    SQLString(info.publisherID), info.duration,
                                    info.visits, info.score, info.percent,
                                    info.weight, info.reconcileStamp];
}

+ (NSString*)contributionInfoInsertFor:(ContributionInfo*)info {
  const auto contributionInfoInsert =
      @"INSERT INTO \"contribution_info\" "
       "(publisher_id, probi, date, type, month, year) VALUES ("
       "%@,"    // publisher_id LONGVARCHAR
       "%@,"    // probi TEXT "0"  NOT NULL
       "%lld,"  // date INTEGER NOT NULL
       "%d,"    // type INTEGER NOT NULL
       "%d,"    // month INTEGER NOT NULL
       "%d"     // year INTEGER NOT NULL
       ");";
  return [NSString stringWithFormat:contributionInfoInsert,
                                    SQLString(info.publisherID),
                                    SQLString(info.probi), info.date, info.type,
                                    info.month, info.year];
}

+ (NSString*)contributionQueueInsertFor:(ContributionQueue*)obj {
  const auto contributionQueueInsert =
      @"INSERT INTO \"contribution_queue\" "
       "(contribution_queue_id, type, amount, partial) VALUES ("
       "%lld,"  // contribution_queue_id INTEGER PRIMARY KEY AUTOINCREMENT NOT
                // NULL
       "%d,"    // type INTEGER NOT NULL
       "%f,"    // amount DOUBLE NOT NULL
       "%d"     // partial INTEGER NOT NULL DEFAULT 0
       ");";
  return [NSString stringWithFormat:contributionQueueInsert, obj.id, obj.type,
                                    obj.amount, obj.partial];
}

+ (NSString*)contributionQueuePublisherInsertFor:(ContributionPublisher*)obj {
  const auto contributionQueuePublisherInsert =
      @"INSERT INTO \"contribution_queue_publishers\" "
       "(contribution_queue_id, publisher_key, amount_percent) VALUES ("
       "%lld,"  // contribution_queue_id INTEGER NOT NULL
       "%@,"    // publisher_key TEXT NOT NULL
       "%f"     // amount_percent DOUBLE NOT NULL
       ");";
  return [NSString stringWithFormat:contributionQueuePublisherInsert,
                                    obj.queue.id, SQLString(obj.publisherKey),
                                    obj.amountPercent];
}

+ (NSString*)mediaPublisherInfoInsertFor:(MediaPublisherInfo*)obj {
  const auto mediaPublisherInfoInsert =
      @"INSERT INTO \"media_publisher_info\" "
       "(media_key, publisher_id) VALUES ("
       "%@,"  // media_key TEXT NOT NULL PRIMARY KEY UNIQUE
       "%@"   // publisher_id LONGVARCHAR NOT NULL
       ");";
  return [NSString stringWithFormat:mediaPublisherInfoInsert,
                                    SQLString(obj.mediaKey),
                                    SQLString(obj.publisherID)];
}

+ (NSString*)pendingContributionInsertFor:(PendingContribution*)obj {
  const auto pendingContributionInsert =
      @"INSERT INTO \"pending_contribution\" "
       "(publisher_id, amount, added_date, viewing_id, type) VALUES ("
       "%@,"    // publisher_id LONGVARCHAR NOT NULL
       "%f,"    // amount DOUBLE DEFAULT 0 NOT NULL
       "%lld,"  // added_date INTEGER DEFAULT 0 NOT NULL
       "%@,"    // viewing_id LONGVARCHAR NOT NULL
       "%d"     // type INTEGER NOT NULL
       ");";
  return [NSString stringWithFormat:pendingContributionInsert,
                                    SQLString(obj.publisherID), obj.amount,
                                    obj.addedDate, SQLString(obj.viewingID),
                                    obj.type];
}

+ (NSString*)promotionInsertFor:(Promotion*)obj {
  const auto promotionInsert =
      @"INSERT INTO \"promotion\" "
       "(promotion_id, version, type, public_keys, suggestions, "
       "approximate_value, status, expires_at) VALUES ("
       "%@,"   // promotion_id TEXT NOT NULL
       "%d,"   // version INTEGER NOT NULL
       "%d,"   // type INTEGER NOT NULL
       "%@,"   // public_keys TEXT NOT NULL
       "%d,"   // suggestions INTEGER NOT NULL DEFAULT 0
       "%f,"   // approximate_value DOUBLE NOT NULL DEFAULT 0
       "%d,"   // status INTEGER NOT NULL DEFAULT 0
       "%lld"  // expires_at TIMESTAMP NOT NULL
       ");";
  return [NSString stringWithFormat:promotionInsert, SQLString(obj.promotionID),
                                    obj.version, obj.type,
                                    SQLString(obj.publicKeys), obj.suggestions,
                                    obj.approximateValue, obj.status,
                                    static_cast<int64_t>(
                                        obj.expiryDate.timeIntervalSince1970)];
}

+ (NSString*)promotionCredsInsertFor:(PromotionCredentials*)obj {
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
  return [NSString
      stringWithFormat:promotionCredsInsert, SQLString(obj.promotionID),
                       SQLString(obj.tokens), SQLString(obj.blindedCredentials),
                       SQLNullableString(obj.signedCredentials),
                       SQLNullableString(obj.publicKey),
                       SQLNullableString(obj.batchProof),
                       SQLNullableString(obj.claimID)];
}

+ (NSString*)publisherInfoInsertFor:(PublisherInfo*)obj {
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
  return
      [NSString stringWithFormat:publisherInfoInsert,
                                 SQLString(obj.publisherID), obj.excluded,
                                 SQLString(obj.name), SQLString(obj.faviconURL),
                                 SQLString(obj.url), SQLString(obj.provider)];
}

+ (NSString*)recurringDonationInsertFor:(RecurringDonation*)obj {
  const auto recurringDonationInsert =
      @"INSERT INTO \"recurring_donation\" "
       "(publisher_id, amount, added_date) VALUES ("
       "%@,"   // publisher_id LONGVARCHAR NOT NULL PRIMARY KEY UNIQUE
       "%f,"   // amount DOUBLE DEFAULT 0 NOT NULL
       "%lld"  // added_date INTEGER DEFAULT 0 NOT NULL
       ");";
  return [NSString stringWithFormat:recurringDonationInsert,
                                    SQLString(obj.publisherID), obj.amount,
                                    obj.addedDate];
}

+ (NSString*)unblindedTokenInsertFor:(UnblindedToken*)obj {
  const auto unblindedTokenInsert =
      @"INSERT INTO \"unblinded_tokens\" "
       "(token_id, token_value, public_key, value, promotion_id) VALUES ("
       "%lld,"  // token_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL
       "%@,"    // token_value TEXT
       "%@,"    // public_key TEXT
       "%f,"    // value DOUBLE NOT NULL DEFAULT 0
       "%@"     // promotion_id TEXT
       ");";
  return [NSString stringWithFormat:unblindedTokenInsert, obj.tokenID,
                                    SQLNullableString(obj.tokenValue),
                                    SQLNullableString(obj.publicKey), obj.value,
                                    SQLNullableString(obj.promotionID)];
}

+ (void)deleteCoreDataServerPublisherList:
    (nullable void (^)(NSError* _Nullable error))completion {
  const auto context = [DataController newBackgroundContext];

  LOG(INFO) << "CoreData: Deleting publisher list";
  [context performBlock:^{
    const auto fetchRequest = ServerPublisherInfo.fetchRequest;
    fetchRequest.entity = [NSEntityDescription
                 entityForName:NSStringFromClass(ServerPublisherInfo.class)
        inManagedObjectContext:context];
    NSError* error = nil;
    const auto deleteRequest =
        [[NSBatchDeleteRequest alloc] initWithFetchRequest:fetchRequest];
    [context executeRequest:deleteRequest error:&error];

    if (!error && context.hasChanges) {
      [context save:&error];
    }

    if (completion) {
      dispatch_async(dispatch_get_main_queue(), ^{
        completion(error);
      });
    }
  }];
}

#pragma mark -

NS_INLINE NSString* SQLNullableString(NSString* _Nullable value) {
  return (value == nil ? @"NULL" : SQLString(value));
}

NS_INLINE NSString* SQLString(NSString* _Nonnull value) {
  // Obj-C doesn't enforce nullability, therefore adding an extra check
  if (value == nil) {
    return @"''";
  }
  // Have to make sure to escape any apostrophies
  return [NSString
      stringWithFormat:@"'%@'",
                       [value stringByReplacingOccurrencesOfString:@"'"
                                                        withString:@"''"]];
}

static NSArray<NSString*>* MapFetchedObjectsToInsertsForClass(
    Class clazz,
    NSString*(NS_NOESCAPE ^ block)(__kindof NSManagedObject* obj)) {
  const auto context = DataController.viewContext;
  const auto fetchRequest = [clazz fetchRequest];
  fetchRequest.entity =
      [NSEntityDescription entityForName:NSStringFromClass(clazz)
                  inManagedObjectContext:context];
  NSError* error;
  const auto fetchedObjects = [context executeFetchRequest:fetchRequest
                                                     error:&error];
  if (error) {
    return @[];
  }
  const auto statements = [[NSMutableArray<NSString*> alloc] init];
  [fetchedObjects
      enumerateObjectsUsingBlock:^(NSManagedObject* _Nonnull obj,
                                   NSUInteger idx, BOOL* _Nonnull stop) {
        if (![obj isKindOfClass:clazz]) {
          return;
        }
        [statements addObject:block(obj)];
      }];
  return statements;
}

@end
