/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_API_H_
#define BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_API_H_

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, BookmarksNodeType) {
  URL,
  FOLDER,
  BOOKMARK_BAR,
  OTHER_NODE,
  MOBILE
};

typedef NS_ENUM(NSUInteger, BookmarksNodeFaviconState) {
  INVALID_FAVICON,
  LOADING_FAVICON,
  LOADED_FAVICON,
};

@interface BookmarkNode: NSObject
+ (NSString *)kRootNodeGuid;
+ (NSString *)kBookmarkBarNodeGuid;
+ (NSString *)kOtherBookmarksNodeGuid;
+ (NSString *)kMobileBookmarksNodeGuid;
+ (NSString *)kManagedNodeGuid;

+ (NSString *)RootNodeGuid;
- (bool)isPermanentNode;
- (void)setTitle:(NSString *)title;
- (NSUInteger)nodeId;
- (void)setNodeId:(NSUInteger)id;
- (NSString *)getGuid;
- (NSURL *)url;
- (void)setUrl:(NSURL *)url;

- (NSURL *)iconUrl;
- (BookmarksNodeType)type;
- (NSDate *)dateAdded;
- (void)setDateAdded:(NSDate *)date;

- (NSDate *)dateFolderModified;
- (void)setDateFolderModified:(NSDate *)date;
- (bool)isFolder;
- (bool)isUrl;
- (bool)isFavIconLoaded;
- (bool)isFavIconLoading;
- (bool)isVisible;

- (bool)getMetaInfo:(NSString *)key value:(NSString **)value;
- (void)setMetaInfo:(NSString *)key value:(NSString *)value;
- (bool)deleteMetaInfo:(NSString *)key;

- (NSString *)titleUrlNodeTitle;
- (NSURL *)titleUrlNodeUrl;
@end

//NS_SWIFT_NAME(BraveBookmarksAPI)
@interface BraveBookmarksAPI: NSObject
- (void)createWithParentId:(NSUInteger)parentId index:(NSUInteger)index title:(NSString *)title url:(NSURL *)url;

- (void)moveWithId:(NSUInteger)bookmarkId parentId:(NSUInteger)parentId index:(NSUInteger)index;

- (void)updateWithId:(NSUInteger)bookmarkId title:(NSString *)title url:(NSURL *)url;

- (void)removeWithId:(NSUInteger)bookmarkId;

- (void)removeAll;

- (NSArray<BookmarkNode *> *)searchWithQuery:(NSString *)query maxCount:(NSUInteger)maxCount;

- (void)undo;

- (void)addBookmark:(NSString *)title url:(NSURL *)url;
@end

#endif  // BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_API_H_
