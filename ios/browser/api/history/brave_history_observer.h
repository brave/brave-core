/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_HISTORY_OBSERVER_H_
#define BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_HISTORY_OBSERVER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class IOSHistoryNode;

OBJC_EXPORT
@protocol HistoryServiceObserver <NSObject>
@optional

/// Notifying when `history_service` has finished loading.
- (void)historyServiceLoaded;

/// Notifying when `history_service` is being deleted.
- (void)historyServiceBeingDeleted;

/// Observing when user visits an URL.
/// @param historyNode - History object which has been visited
- (void)historyNodeVisited:(IOSHistoryNode*)historyNode;

/// Observing when a URL is added or modified
/// @param historyNodeList - History object list which has been modified
- (void)historyNodesModified:(NSArray<IOSHistoryNode*>*)historyNodeList;

/// Observing when one or more URLs are deleted
/// @param historyNodeList - History object list which has been deleted
///                          historyNodeList will be empty is all history is
///                          deleted
/// @param isAllHistory - Boolean that notifies all history is deleted
- (void)historyNodesDeleted:(NSArray<IOSHistoryNode*>*)historyNodeList
               isAllHistory:(bool)isAllHistory;
@end

OBJC_EXPORT
@protocol HistoryServiceListener
- (void)destroy;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_HISTORY_BRAVE_HISTORY_OBSERVER_H_
