/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_API_H_
#define BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_API_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

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

@protocol BookmarkModelObserver;
@protocol BookmarkModelListener;

OBJC_EXPORT
@interface BookmarkNode: NSObject
+ (NSString *)kRootNodeGuid;
+ (NSString *)kBookmarkBarNodeGuid;
+ (NSString *)kOtherBookmarksNodeGuid;
+ (NSString *)kMobileBookmarksNodeGuid;
+ (NSString *)kManagedNodeGuid;

- (bool)isPermanentNode;
- (void)setTitle:(NSString *)title;
- (NSUInteger)nodeId;
//- (void)setNodeId:(NSUInteger)id;
- (NSString *)getGuid;
- (nullable NSURL *)url;
- (void)setUrl:(nullable NSURL *)url;

- (nullable NSURL *)iconUrl;
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

- (bool)getMetaInfo:(NSString *)key value:(NSString * _Nonnull * _Nullable)value;
- (void)setMetaInfo:(NSString *)key value:(NSString *)value;
- (void)deleteMetaInfo:(NSString *)key;

- (NSString *)titleUrlNodeTitle;
- (nullable NSURL *)titleUrlNodeUrl;

- (nullable BookmarkNode *)parent;
- (NSArray<BookmarkNode *> *)children;

- (nullable BookmarkNode *)addChildFolderWithTitle:(NSString *)title;
- (nullable BookmarkNode *)addChildBookmarkWithTitle:(NSString *)title url:(NSURL *)url;

- (void)moveToParent:(nonnull BookmarkNode *)parent;
- (void)moveToParent:(nonnull BookmarkNode *)parent index:(NSUInteger)index;
@end

//NS_SWIFT_NAME(BraveBookmarksAPI)
OBJC_EXPORT
@interface BraveBookmarksAPI: NSObject

@property(class, readonly, strong) BraveBookmarksAPI *sharedBookmarksAPI NS_SWIFT_NAME(shared);

- (nullable BookmarkNode *)rootNode;

- (nullable BookmarkNode *)otherNode;

- (nullable BookmarkNode *)mobileNode;

- (nullable BookmarkNode *)desktopNode;

- (bool)isLoaded;

- (id<BookmarkModelListener>)addObserver:(id<BookmarkModelObserver>)observer;
- (void)removeObserver:(id<BookmarkModelListener>)observer;

- (bool)isEditingEnabled;

- (nullable BookmarkNode *)createFolderWithTitle:(NSString *)title;
- (nullable BookmarkNode *)createFolderWithParent:(BookmarkNode *)parent title:(NSString *)title;

- (nullable BookmarkNode *)createBookmarkWithTitle:(NSString *)title url:(NSURL *)url;
- (nullable BookmarkNode *)createBookmarkWithParent:(BookmarkNode *)parent title:(NSString *)title withUrl:(NSURL *)url;

- (void)removeBookmark:(BookmarkNode *)bookmark;
- (void)removeAll;

- (NSArray<BookmarkNode *> *)searchWithQuery:(NSString *)query maxCount:(NSUInteger)maxCount;

- (void)undo;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_API_H_
