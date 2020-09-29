/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_API_H_
#define BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_API_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, BookmarksNodeType) {
  BookmarksNodeTypeURL,
  BookmarksNodeTypeFOLDER,
  BookmarksNodeTypeBOOKMARK_BAR,
  BookmarksNodeTypeOTHER_NODE,
  BookmarksNodeTypeMOBILE
};

typedef NS_ENUM(NSUInteger, BookmarksNodeFaviconState) {
  BookmarksNodeFaviconStateINVALID_FAVICON,
  BookmarksNodeFaviconStateLOADING_FAVICON,
  BookmarksNodeFaviconStateLOADED_FAVICON,
};

@protocol BookmarkModelObserver;
@protocol BookmarkModelListener;

OBJC_EXPORT
@interface BookmarkNode: NSObject
@property (class, nonatomic, copy, readonly, getter=kRootNodeGuid) NSString *rootNodeGuid;
@property (class, nonatomic, copy, readonly, getter=kBookmarkBarNodeGuid) NSString *bookmarkBarNodeGuid;
@property (class, nonatomic, copy, readonly, getter=kOtherBookmarksNodeGuid) NSString *otherBookmarksNodeGuid;
@property (class, nonatomic, copy, readonly, getter=kMobileBookmarksNodeGuid) NSString *mobileBookmarksNodeGuid;
@property (class, nonatomic, copy, readonly, getter=kManagedNodeGuid) NSString *managedNodeGuid;

@property (nonatomic, assign, readonly) bool isPermanentNode;

@property (nonatomic, assign, readonly, getter=getNodeId) NSUInteger nodeId;
@property (nonatomic, copy, readonly, getter=getGuid) NSString *guid;
@property (nonatomic, nullable, copy, getter=getUrl) NSURL *url;
@property (nonatomic, nullable, copy, readonly, getter=getIconUrl) NSURL *iconUrl;

@property (nonatomic, assign, readonly, getter=getType) BookmarksNodeType type;
@property (nonatomic, copy, getter=getDateAdded) NSDate *dateAdded;
@property (nonatomic, copy, getter=getDateFolderModified) NSDate *dateFolderModified;

@property (nonatomic, assign, readonly) bool isFolder;
@property (nonatomic, assign, readonly) bool isUrl;
@property (nonatomic, assign, readonly) bool isFavIconLoaded;
@property (nonatomic, assign, readonly) bool isFavIconLoading;
@property (nonatomic, assign, readonly) bool isVisible;

@property (nonatomic, readonly, getter=getTitleUrlNodeTitle) NSString *titleUrlNodeTitle;
@property (nonatomic, nullable, readonly, getter=getTitleUrlNodeUrl) NSURL *titleUrlNodeUrl;

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
@property (nonatomic, nullable, readonly, getter=getRootNode) BookmarkNode *rootNode;
@property (nonatomic, nullable, readonly, getter=getOtherNode) BookmarkNode *otherNode;
@property (nonatomic, nullable, readonly, getter=getMobileNode) BookmarkNode *mobileNode;
@property (nonatomic, nullable, readonly, getter=getDesktopNode) BookmarkNode *desktopNode;
@property (nonatomic, assign, readonly) bool isLoaded;
@property (nonatomic, assign, readonly, getter=isEditingEnabled) bool editingEnabled;

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
