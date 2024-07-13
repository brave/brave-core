/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_HISTORY_API_H_
#define BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_HISTORY_API_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@protocol HistoryServiceObserver;
@protocol HistoryServiceListener;

@class IOSHistoryNode;

typedef NSInteger DomainMetricTypeIOS
    NS_TYPED_ENUM NS_SWIFT_NAME(DomainMetricType);
OBJC_EXPORT DomainMetricTypeIOS const DomainMetricTypeIOSNoMetric;
OBJC_EXPORT DomainMetricTypeIOS const DomainMetricTypeIOSLast1DayMetric;
OBJC_EXPORT DomainMetricTypeIOS const DomainMetricTypeIOSLast7DayMetric;
OBJC_EXPORT DomainMetricTypeIOS const DomainMetricTypeIOSLast28DayMetric;

NS_SWIFT_NAME(HistoryDuplicateHandling)
typedef NS_ENUM(NSInteger, HistoryDuplicateHandlingIOS) {
  HistoryDuplicateHandlingIOSRemoveAll,
  HistoryDuplicateHandlingIOSRemovePerDay,
  HistoryDuplicateHandlingIOSKeepAll
};

NS_SWIFT_NAME(HistoryNode)
OBJC_EXPORT
@interface IOSHistoryNode : NSObject

@property(nonatomic, strong, readonly) NSURL* url;
@property(nonatomic, nullable, copy) NSString* title;
@property(nonatomic, nullable, copy) NSDate* dateAdded;

/// History Node Constructor used with HistoryAPI
/// @param url - Mandatory URL field for the history object
/// @param title - Title used for the URL
/// @param dateAdded - Date History Object is created
- (instancetype)initWithURL:(NSURL*)url
                      title:(nullable NSString*)title
                  dateAdded:(nullable NSDate*)dateAdded;
@end

NS_SWIFT_NAME(HistorySearchOptions)
OBJC_EXPORT
@interface IOSHistorySearchOptions : NSObject
/// The maximum number of results to return. The results will be sorted with
/// the most recent first, so older results may not be returned if there is not
/// enough room. When 0, this will return everything.
@property(nonatomic) NSUInteger maxCount;
/// Whether the history query should only search through hostnames.
/// When this is true, the matching_algorithm field is ignored.
@property(nonatomic) BOOL hostOnly;
/// Allows the caller to specify how duplicate URLs in the result set should
/// be handled.
@property(nonatomic) HistoryDuplicateHandlingIOS duplicateHandling;
/// Query only items added after this date
/// When `visit_order` is `RECENT_FIRST`, the beginning is inclusive.
/// When `VisitOrder` is `OLDEST_FIRST`, vice versa.
///
/// This will match only the one recent visit of a URL. For text search
/// queries, if the URL was visited in the given time period, but has also
/// been visited more recently than that, it will not be returned. When the
/// text query is empty, this will return the most recent visit within the
/// time range.
@property(nonatomic, nullable, copy) NSDate* beginDate;
/// Query only items added before this date
/// When `visit_order` is `RECENT_FIRST`, the the ending is exclusive.
/// When `VisitOrder` is `OLDEST_FIRST`, vice versa.
///
/// This will match only the one recent visit of a URL. For text search
/// queries, if the URL was visited in the given time period, but has also
/// been visited more recently than that, it will not be returned. When the
/// text query is empty, this will return the most recent visit within the
/// time range.
@property(nonatomic, nullable, copy) NSDate* endDate;

/// History Search Options Constructor used with HistoryAPI
- (instancetype)init;

/// History Search Options Constructor used with HistoryAPI
/// @param maxCount - Maximum number of items requested
/// @param duplicateHandling - Specifies how duplicates should be handled
- (instancetype)initWithMaxCount:(NSUInteger)maxCount
               duplicateHandling:(HistoryDuplicateHandlingIOS)duplicateHandling;

/// History Search Options Constructor used with HistoryAPI
/// @param maxCount - Maximum number of items requested
/// @param hostOnly - Use the host only for the search
/// @param duplicateHandling - Specifies how duplicates should be handled
/// @param beginDate - Query only items added after this date
/// @param endDate - Query only items added before this date
- (instancetype)initWithMaxCount:(NSUInteger)maxCount
                        hostOnly:(BOOL)hostOnly
               duplicateHandling:(HistoryDuplicateHandlingIOS)duplicateHandling
                       beginDate:(nullable NSDate*)beginDate
                         endDate:(nullable NSDate*)endDate;

@end

NS_SWIFT_NAME(BraveHistoryAPI)
OBJC_EXPORT
@interface BraveHistoryAPI : NSObject

@property(nonatomic, readonly) bool isBackendLoaded;

- (id<HistoryServiceListener>)addObserver:(id<HistoryServiceObserver>)observer;
- (void)removeObserver:(id<HistoryServiceListener>)observer;

- (instancetype)init NS_UNAVAILABLE;

/// Add History Method which also allows to edit transition type
/// @param history - History Object to be added
- (void)addHistory:(IOSHistoryNode*)history;

/// Remove Specific History
/// @param node - HistoryNode object to be removed from history
- (void)removeHistoryForNode:(IOSHistoryNode*)node;

/// Remove Specific History for an array of nodes
/// @param nodes - An array of HistoryNode objects to be removed from history
- (void)removeHistoryForNodes:(NSArray<IOSHistoryNode*>*)nodes;

/// Remove All History
/// @param completion - Block that notifies removing is finished
- (void)removeAllWithCompletion:(void (^)())completion;

/// Query Function providing history items array in completion block
/// @param query - Search Query (Empty Query returns all History)
/// @param options - Additional search options
/// @param completion - Block that notifies querying is finished with list of
/// items
- (void)searchWithQuery:(NSString* _Nullable)query
                options:(IOSHistorySearchOptions*)searchOptions
             completion:
                 (void (^)(NSArray<IOSHistoryNode*>* historyResults))completion;

/// Gets a count of unique domains visited as of now based on the `type` passed
- (void)fetchDomainDiversityForType:(DomainMetricTypeIOS)type
                         completion:(void (^)(NSInteger count))completion;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_HISTORY_API_H_
