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

NS_SWIFT_NAME(BraveHistoryAPI)
OBJC_EXPORT
@interface BraveHistoryAPI : NSObject

@property(nonatomic, readonly) bool isBackendLoaded;

- (id<HistoryServiceListener>)addObserver:(id<HistoryServiceObserver>)observer;
- (void)removeObserver:(id<HistoryServiceListener>)observer;

- (instancetype)init NS_UNAVAILABLE;

/// Add History Method which also allows to edit transition type
/// @param history - History Object to be added
/// @param isURLTyped - Bool determine If URL is typed and synced
- (void)addHistory:(IOSHistoryNode*)history isURLTyped:(BOOL)isURLTyped;

/// Remove Specific History
/// @param history - History Object to be removed from history
- (void)removeHistory:(IOSHistoryNode*)history;

/// Remove All History
/// @param completion - Block that notifies removing is finished
- (void)removeAllWithCompletion:(void (^)())completion;

/// Query Function providing history items array in completion block
/// @param query - Search Query (Empty Query returns all History)
/// @param maxCount - Number of items requested
/// @param completion - Block that notifies querying is finished with list of
/// items
- (void)searchWithQuery:(NSString* _Nullable)query
               maxCount:(NSUInteger)maxCount
             completion:
                 (void (^)(NSArray<IOSHistoryNode*>* historyResults))completion;

/// Gets a count of unique domains visited as of now based on the `type` passed
- (void)fetchDomainDiversityForType:(DomainMetricTypeIOS)type
                         completion:(void (^)(NSInteger count))completion;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_HISTORY_API_H_
