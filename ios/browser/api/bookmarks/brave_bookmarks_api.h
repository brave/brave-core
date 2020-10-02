/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_API_H_
#define BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_API_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, BookmarksNodeType) {
  BookmarksNodeTypeUrl,
  BookmarksNodeTypeFolder,
  BookmarksNodeTypeBookmarkBar,
  BookmarksNodeTypeOtherNode,
  BookmarksNodeTypeMobile
};

typedef NS_ENUM(NSUInteger, BookmarksNodeFaviconState) {
  BookmarksNodeFaviconStateInvalidFavIcon,
  BookmarksNodeFaviconStateLoadingFavIcon,
  BookmarksNodeFaviconStateLoadedFavIcon,
};

@protocol BookmarkModelObserver;
@protocol BookmarkModelListener;

OBJC_EXPORT
@interface BookmarkNode: NSObject
@property (class, nonatomic, copy, readonly) NSString *rootNodeGuid;
@property (class, nonatomic, copy, readonly) NSString *bookmarkBarNodeGuid;
@property (class, nonatomic, copy, readonly) NSString *otherBookmarksNodeGuid;
@property (class, nonatomic, copy, readonly) NSString *mobileBookmarksNodeGuid;
@property (class, nonatomic, copy, readonly) NSString *managedNodeGuid;

@property (nonatomic, readonly) bool isPermanentNode;

@property (nonatomic, readonly) NSUInteger nodeId;
@property (nonatomic, copy, readonly) NSString *guid;
@property (nonatomic, nullable, copy) NSURL *url;
@property (nonatomic, nullable, copy, readonly) NSURL *iconUrl;

@property (nonatomic, readonly) BookmarksNodeType type;
@property (nonatomic, copy) NSDate *dateAdded;
@property (nonatomic, copy) NSDate *dateFolderModified;

@property (nonatomic, readonly) bool isFolder;
@property (nonatomic, readonly) bool isUrl;
@property (nonatomic, readonly) bool isFavIconLoaded;
@property (nonatomic, readonly) bool isFavIconLoading;
@property (nonatomic, readonly) bool isVisible;

@property (nonatomic, readonly) NSString *titleUrlNodeTitle;
@property (nonatomic, nullable, readonly) NSURL *titleUrlNodeUrl;

@property (nonatomic, nullable, readonly) BookmarkNode *parent;
@property (nonatomic, readonly) NSArray<BookmarkNode *> *children;

- (void)setTitle:(NSString *)title;
- (bool)getMetaInfo:(NSString *)key value:(NSString * _Nonnull * _Nullable)value;
- (void)setMetaInfo:(NSString *)key value:(NSString *)value;
- (void)deleteMetaInfo:(NSString *)key;

- (nullable BookmarkNode *)addChildFolderWithTitle:(NSString *)title;
- (nullable BookmarkNode *)addChildBookmarkWithTitle:(NSString *)title url:(NSURL *)url;

- (void)moveToParent:(nonnull BookmarkNode *)parent;
- (void)moveToParent:(nonnull BookmarkNode *)parent index:(NSUInteger)index;
@end

//NS_SWIFT_NAME(BraveBookmarksAPI)
OBJC_EXPORT
@interface BraveBookmarksAPI: NSObject

@property (class, readonly, getter=sharedBookmarksAPI) BraveBookmarksAPI *shared;
@property (nonatomic, nullable, readonly) BookmarkNode *rootNode;
@property (nonatomic, nullable, readonly) BookmarkNode *otherNode;
@property (nonatomic, nullable, readonly) BookmarkNode *mobileNode;
@property (nonatomic, nullable, readonly) BookmarkNode *desktopNode;
@property (nonatomic, readonly) bool isLoaded;
@property (nonatomic, readonly) bool editingEnabled;

- (id<BookmarkModelListener>)addObserver:(id<BookmarkModelObserver>)observer;
- (void)removeObserver:(id<BookmarkModelListener>)observer;

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
