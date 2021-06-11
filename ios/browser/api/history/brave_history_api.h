/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_HISTORY_API_H_
#define BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_HISTORY_API_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, BraveHistoryTransitionType) {
  BraveHistoryTransitionType_LINK,
  BraveHistoryTransitionType_TYPED,
};

@protocol HistoryServiceObserver;
@protocol HistoryServiceListener;

@class IOSHistoryNode;

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
                      title:(NSString* _Nullable)title
                  dateAdded:(NSDate* _Nullable)dateAdded;
@end

NS_SWIFT_NAME(BraveHistoryAPI)
OBJC_EXPORT
@interface BraveHistoryAPI : NSObject

@property(class, readonly, getter = sharedHistoryAPI) BraveHistoryAPI* shared;
@property(nonatomic, readonly) bool isLoaded;

- (id<HistoryServiceListener>)addObserver:(id<HistoryServiceObserver>)observer;
- (void)removeObserver:(id<HistoryServiceListener>)observer;

/// Default Add History Method which also syncs typed URLS
/// @param history - History Object to be added
- (void)addHistory:(IOSHistoryNode*)history;

/// Add History Method which also allows to edit transition type
/// @param history - History Object to be added
/// @param pageTransition - History Object to be added
- (void)addHistory:(IOSHistoryNode*)history
    pageTransition:(BraveHistoryTransitionType)pageTransition;

/// Remove Specific History
/// @param history - History Object to be removed from history
- (void)removeHistory:(IOSHistoryNode*)history;

/// Remove All History
/// @param completion - Block that notifies removing is finished
- (void)removeAllWithCompletion:(void(^)())completion;

/// Query Function providing history items array in completion block
/// @param query - Search Query (Empty Query returns all History)
/// @param maxCount - Number of items requested
/// @param completion - Block that notifies querying is finished with list of
/// items
- (void)searchWithQuery:(NSString* _Nullable)query
               maxCount:(NSUInteger)maxCount
             completion:
                 (void (^)(NSArray<IOSHistoryNode*>* historyResults))completion;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_HISTORY_API_H_
