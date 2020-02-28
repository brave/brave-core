/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "Records.h"
#import "ledger.mojom.objc.h"

NS_ASSUME_NONNULL_BEGIN

typedef void (^BATLedgerDatabaseWriteCompletion)(BOOL success);

/// An interface into the ledger database
///
/// This class mirrors brave-core's `publisher_info_database.h/cc` file. This file will actually
/// likely be removed at a future date when database managment happens in the ledger library
@interface BATLedgerDatabase : NSObject

/// Generates a SQL migration transaction that will move all data in the users
/// CoreData storage into version 10 of the brave-core's database schema to
/// then run and have ledger take over
///
/// Return's nil if the migration template cannot be found
+ (nullable NSString *)migrateCoreDataToSQLTransaction;

// TO BE REMOVED:

#pragma mark - Contribution Info

/// Insert or update contribution info into the database given all the information for a contribution
+ (void)insertContributionInfo:(NSString *)probi
                         month:(const BATActivityMonth)month
                          year:(const int)year
                          date:(const uint32_t)date
                  publisherKey:(NSString *)publisherKey
                          type:(BATRewardsType)type
                    completion:(nullable BATLedgerDatabaseWriteCompletion)completion;

/// Get a list of publishers you have supported with one time tips given some month and year
+ (NSArray<BATPublisherInfo *> *)oneTimeTipsPublishersForMonth:(BATActivityMonth)month
                                                          year:(int)year;

#pragma mark - Contribution Queue

+ (void)insertOrUpdateContributionQueue:(BATContributionQueue *)queue
                             completion:(nullable BATLedgerDatabaseWriteCompletion)completion;

+ (nullable BATContributionQueue *)firstQueue;

+ (void)deleteQueueWithID:(int64_t)queueID
               completion:(nullable BATLedgerDatabaseWriteCompletion)completion;

#pragma mark - Media Publisher Info

/// Get a publisher linked with some media key
+ (nullable BATPublisherInfo *)mediaPublisherInfoWithMediaKey:(NSString *)mediaKey;

/// Insert or update some media info given some media key and publisher ID that it is linked to
+ (void)insertOrUpdateMediaPublisherInfoWithMediaKey:(NSString *)mediaKey
                                         publisherID:(NSString *)publisherID
                                          completion:(nullable BATLedgerDatabaseWriteCompletion)completion;

#pragma mark - Recurring Tips

/// Get a list of publishers you have supported with recurring tips
+ (NSArray<BATPublisherInfo *> *)recurringTips;

/// Insert a recurring tip linked to a given publisher ID for some amount
+ (void)insertOrUpdateRecurringTipWithPublisherID:(NSString *)publisherID
                                           amount:(double)amount
                                        dateAdded:(uint32_t)dateAdded
                                       completion:(nullable BATLedgerDatabaseWriteCompletion)completion;

/// Remove a recurring tip linked to a given publisher ID
+ (void)removeRecurringTipWithPublisherID:(NSString *)publisherID
                               completion:(nullable BATLedgerDatabaseWriteCompletion)completion;

#pragma mark - Pending Contributions

/// Get a list of pending contributions
+ (NSArray<BATPendingContributionInfo *> *)pendingContributions;

/// Inserts a set of pending contributions from a contribution list
+ (void)insertPendingContributions:(NSArray<BATPendingContribution *> *)contributions
                        completion:(nullable BATLedgerDatabaseWriteCompletion)completion;

/// Remove a pending contribution for a given publisher, viewing ID and added date
+ (void)removePendingContributionForPublisherID:(NSString *)publisherID
                                      viewingID:(NSString *)viewingID
                                      addedDate:(UInt64)addedDate
                                     completion:(nullable BATLedgerDatabaseWriteCompletion)completion;

/// Removes all the users pending contributions
+ (void)removeAllPendingContributions:(nullable BATLedgerDatabaseWriteCompletion)completion;

/// Get the amount of BAT allocated for pending contributions
+ (double)reservedAmountForPendingContributions;

#pragma mark - Publisher List

+ (nullable BATServerPublisherInfo *)serverPublisherInfoWithPublisherID:(NSString *)publisherID;

+ (void)clearAndInsertList:(NSArray<BATServerPublisherInfo *> *)list
                completion:(nullable BATLedgerDatabaseWriteCompletion)completion;

#pragma mark - Publisher List / Test Helpers

+ (nullable BATPublisherBanner *)bannerForPublisherID:(NSString *)publisherID;
+ (nullable NSArray<NSNumber *> *)bannerAmountsForPublisherWithPublisherID:(NSString *)publisherID;
+ (nullable NSDictionary<NSString *, NSString *> *)publisherLinksWithPublisherID:(NSString *)publisherID;

#pragma mark - Promotions

+ (NSArray<BATPromotion *> *)allPromotions;

+ (void)insertOrUpdatePromotion:(BATPromotion *)promotion
                     completion:(nullable BATLedgerDatabaseWriteCompletion)completion;

+ (nullable BATPromotion *)promotionWithID:(NSString *)promoID;

+ (void)insertOrUpdateUnblindedToken:(BATUnblindedToken *)unblindedToken
                          completion:(nullable BATLedgerDatabaseWriteCompletion)completion;

+ (NSArray<BATUnblindedToken *> *)allUnblindedTokens;

+ (void)deleteUnblindedTokens:(NSArray<NSNumber *> *)idList
                   completion:(nullable BATLedgerDatabaseWriteCompletion)completion;

#pragma mark -

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder *)aDecoder NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
