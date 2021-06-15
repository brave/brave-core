/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_API_H_
#define BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_API_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

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

@class IOSBookmarkNode;

OBJC_EXPORT
@interface BookmarkFolder : NSObject
@property(nonatomic, readonly) IOSBookmarkNode* bookmarkNode;
@property(nonatomic, readonly) NSInteger indentationLevel;
@end

NS_SWIFT_NAME(BookmarkNode)
OBJC_EXPORT
@interface IOSBookmarkNode : NSObject
@property(class, nonatomic, copy, readonly) NSString* rootNodeGuid;
@property(class, nonatomic, copy, readonly) NSString* bookmarkBarNodeGuid;
@property(class, nonatomic, copy, readonly) NSString* otherBookmarksNodeGuid;
@property(class, nonatomic, copy, readonly) NSString* mobileBookmarksNodeGuid;
@property(class, nonatomic, copy, readonly) NSString* managedNodeGuid;

@property(nonatomic, readonly) bool isPermanentNode;

@property(nonatomic, readonly) NSUInteger nodeId;
@property(nonatomic, copy, readonly) NSString* guid;
@property(nonatomic, nullable, copy) NSURL* url;
@property(nonatomic, nullable, copy, readonly) NSURL* iconUrl;
@property(nonatomic, nullable, copy, readonly) UIImage* icon;

@property(nonatomic, readonly) BookmarksNodeType type;
@property(nonatomic, copy) NSDate* dateAdded;
@property(nonatomic, copy) NSDate* dateFolderModified;

@property(nonatomic, readonly) bool isFolder;
@property(nonatomic, readonly) bool isUrl;
@property(nonatomic, readonly) bool isFavIconLoaded;
@property(nonatomic, readonly) bool isFavIconLoading;
@property(nonatomic, readonly) bool isVisible;
@property(nonatomic, readonly) bool isValid;

@property(nonatomic, readonly) NSString* titleUrlNodeTitle;
@property(nonatomic, nullable, readonly) NSURL* titleUrlNodeUrl;

@property(nonatomic, nullable, readonly) IOSBookmarkNode* parent;
@property(nonatomic, readonly) NSArray<IOSBookmarkNode*>* children;
@property(nonatomic, readonly) NSArray<BookmarkFolder*>* nestedChildFolders;
@property(nonatomic, readonly) NSUInteger childCount;

- (nullable IOSBookmarkNode*)childAtIndex:(NSUInteger)index;
- (NSArray<BookmarkFolder*>*)nestedChildFoldersFiltered:
                                          (BOOL(^)(BookmarkFolder*))included
                                  NS_SWIFT_NAME(nestedChildFolders(where:));

- (void)setTitle:(NSString*)title;
- (bool)getMetaInfo:(NSString*)key value:(NSString* _Nonnull* _Nullable)value;
- (void)setMetaInfo:(NSString*)key value:(NSString*)value;
- (void)deleteMetaInfo:(NSString*)key;

- (nullable IOSBookmarkNode*)addChildFolderWithTitle:(NSString*)title;
- (nullable IOSBookmarkNode*)addChildBookmarkWithTitle:(NSString*)title
                                                url:(NSURL*)url;

- (void)moveToParent:(nonnull IOSBookmarkNode*)parent;
- (void)moveToParent:(nonnull IOSBookmarkNode*)parent index:(NSUInteger)index;
- (NSInteger)indexOfChild:(nonnull IOSBookmarkNode*)child;
- (bool)hasAncestor:(nonnull IOSBookmarkNode*)parent;

- (instancetype)initWithTitle:(NSString*)title
                           id:(int64_t)id
                         guid:(NSString* _Nullable)guid
                          url:(NSURL* _Nullable)url
                    dateAdded:(NSDate* _Nullable)dateAdded
                 dateModified:(NSDate* _Nullable)dateModified
                     children:
                         (NSArray<IOSBookmarkNode*>* _Nullable)children;
@end

NS_SWIFT_NAME(BraveBookmarksAPI)
OBJC_EXPORT
@interface BraveBookmarksAPI : NSObject
@property(class, readonly, getter = sharedBookmarksAPI)
    BraveBookmarksAPI* shared;
@property(nonatomic, nullable, readonly) IOSBookmarkNode* rootNode;
@property(nonatomic, nullable, readonly) IOSBookmarkNode* otherNode;
@property(nonatomic, nullable, readonly) IOSBookmarkNode* mobileNode;
@property(nonatomic, nullable, readonly) IOSBookmarkNode* desktopNode;
@property(nonatomic, readonly) bool isLoaded;
@property(nonatomic, readonly) bool editingEnabled;

- (id<BookmarkModelListener>)addObserver:(id<BookmarkModelObserver>)observer;
- (void)removeObserver:(id<BookmarkModelListener>)observer;

- (nullable IOSBookmarkNode*)createFolderWithTitle:(NSString*)title;
- (nullable IOSBookmarkNode*)createFolderWithParent:(IOSBookmarkNode*)parent
                                              title:(NSString*)title;

- (nullable IOSBookmarkNode*)createBookmarkWithTitle:(NSString*)title
                                                 url:(NSURL*)url;
- (nullable IOSBookmarkNode*)createBookmarkWithParent:(IOSBookmarkNode*)parent
                                                title:(NSString*)title
                                              withUrl:(NSURL*)url;

- (nullable IOSBookmarkNode*)getNodeById:(NSInteger)nodeId;

- (void)removeBookmark:(IOSBookmarkNode*)bookmark;
- (void)removeAll;

- (void)searchWithQuery:(NSString*)query
               maxCount:(NSUInteger)maxCount
             completion:(void (^)(NSArray<IOSBookmarkNode*>*))completion;

- (void)undo;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_API_H_
