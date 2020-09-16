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

OBJC_EXPORT
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
//- (void)setNodeId:(NSUInteger)id;
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
- (void)deleteMetaInfo:(NSString *)key;

- (NSString *)titleUrlNodeTitle;
- (NSURL *)titleUrlNodeUrl;

- (BookmarkNode *)parent;
- (NSArray<BookmarkNode *> *)children;

- (BookmarkNode *)addChildFolderWithTitle:(NSString *)title;
- (BookmarkNode *)addChildBookmarkWithTitle:(NSString *)title url:(NSURL *)url;

- (void)moveToParent:(BookmarkNode *)parent;
- (void)moveToParent:(BookmarkNode *)parent index:(NSUInteger)index;
@end

//NS_SWIFT_NAME(BraveBookmarksAPI)
OBJC_EXPORT
@interface BraveBookmarksAPI: NSObject

- (BookmarkNode *)rootNode;

- (BookmarkNode *)otherNode;

- (BookmarkNode *)mobileNode;

- (BookmarkNode *)desktopNode;

- (bool)isEditingEnabled;

- (BookmarkNode *)createFolderWithTitle:(NSString *)title;
- (BookmarkNode *)createFolderWithParent:(BookmarkNode *)parent title:(NSString *)title;

- (BookmarkNode *)createBookmarkWithTitle:(NSString *)title url:(NSURL *)url;
- (BookmarkNode *)createBookmarkWithParent:(BookmarkNode *)parent title:(NSString *)title withUrl:(NSURL *)url;

- (void)removeBookmark:(BookmarkNode *)bookmark;
- (void)removeAll;

- (NSArray<BookmarkNode *> *)searchWithQuery:(NSString *)query maxCount:(NSUInteger)maxCount;

- (void)undo;
@end

#endif  // BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_API_H_
