/* Copyright (c) 2020 The Brave Authors. All rights reserved.
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_OBSERVER_H_
#define BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_OBSERVER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class BookmarkNode;

OBJC_EXPORT
@protocol BookmarkModelObserver<NSObject>
- (void)bookmarkModelLoaded;

- (void)bookmarkNodeChanged:(BookmarkNode *)bookmarkNode;

- (void)bookmarkNodeChildrenChanged:
    (BookmarkNode *)bookmarkNode;

- (void)bookmarkNode:(BookmarkNode *)bookmarkNode
     movedFromParent:(BookmarkNode *)oldParent
            toParent:(BookmarkNode *)newParent;

- (void)bookmarkNodeDeleted:(BookmarkNode *)node
                 fromFolder:(BookmarkNode *)folder;

- (void)bookmarkModelRemovedAllNodes;

@optional
- (void)bookmarkNodeFaviconChanged:(BookmarkNode *)bookmarkNode;
@end

OBJC_EXPORT
@protocol BookmarkModelListener
- (void)destroy;
@end

OBJC_EXPORT
@interface BookmarkModelListenerImpl: NSObject<BookmarkModelListener>
- (instancetype)init:(id<BookmarkModelObserver>)observer bookmarkModel:(void *)bookmarkModel;
@end

NS_ASSUME_NONNULL_END

#endif /* BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_OBSERVER_H_ */
