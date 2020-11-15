/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/bookmarks/brave_bookmarks_api.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/api/bookmarks/brave_bookmarks_observer.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/undo/bookmark_undo_service.h"
#include "components/undo/undo_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/undo/bookmark_undo_service_factory.h"
#include "ios/web/public/thread/web_thread.h"
#import "net/base/mac/url_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface BookmarkNode () {
  const bookmarks::BookmarkNode* node_;  // UNOWNED
  bookmarks::BookmarkModel* model_;      // UNOWNED
}
@end

@implementation BookmarkNode

- (instancetype)initWithNode:(const bookmarks::BookmarkNode*)node
                       model:(bookmarks::BookmarkModel*)model {
  if ((self = [super init])) {
    self->node_ = node;
    self->model_ = model;
  }
  return self;
}

- (void)dealloc {
  self->node_ = nullptr;
  self->model_ = nullptr;
}

+ (NSString*)rootNodeGuid {
  return base::SysUTF8ToNSString(bookmarks::BookmarkNode::kRootNodeGuid);
}

+ (NSString*)bookmarkBarNodeGuid {
  return base::SysUTF8ToNSString(bookmarks::BookmarkNode::kBookmarkBarNodeGuid);
}

+ (NSString*)otherBookmarksNodeGuid {
  return base::SysUTF8ToNSString(
      bookmarks::BookmarkNode::kOtherBookmarksNodeGuid);
}

+ (NSString*)mobileBookmarksNodeGuid {
  return base::SysUTF8ToNSString(
      bookmarks::BookmarkNode::kMobileBookmarksNodeGuid);
}

+ (NSString*)managedNodeGuid {
  return base::SysUTF8ToNSString(bookmarks::BookmarkNode::kManagedNodeGuid);
}

- (bool)isPermanentNode {
  DCHECK(node_);
  return node_->is_permanent_node();
}

- (void)setTitle:(NSString*)title {
  DCHECK(node_);
  model_->SetTitle(node_, base::SysNSStringToUTF16(title));
}

- (NSUInteger)nodeId {
  DCHECK(node_);
  return node_->id();
}

- (NSString*)guid {
  DCHECK(node_);
  return base::SysUTF8ToNSString(node_->guid());
}

- (NSURL*)url {
  DCHECK(node_);
  return net::NSURLWithGURL(node_->url());
}

- (void)setUrl:(NSURL*)url {
  DCHECK(node_);
  model_->SetURL(node_, net::GURLWithNSURL(url));
}

- (NSURL*)iconUrl {
  DCHECK(node_);
  const GURL* url = node_->icon_url();
  return url ? net::NSURLWithGURL(*url) : nullptr;
}

- (UIImage*)icon {
  DCHECK(node_);
  gfx::Image icon = model_->GetFavicon(node_);
  return icon.IsEmpty() ? nullptr : icon.ToUIImage();
}

- (BookmarksNodeType)type {
  DCHECK(node_);
  switch (node_->type()) {
    case bookmarks::BookmarkNode::URL:
      return BookmarksNodeTypeUrl;
    case bookmarks::BookmarkNode::FOLDER:
      return BookmarksNodeTypeFolder;
    case bookmarks::BookmarkNode::BOOKMARK_BAR:
      return BookmarksNodeTypeBookmarkBar;
    case bookmarks::BookmarkNode::OTHER_NODE:
      return BookmarksNodeTypeOtherNode;
    case bookmarks::BookmarkNode::MOBILE:
      return BookmarksNodeTypeMobile;
  }
  return BookmarksNodeTypeMobile;
}

- (NSDate*)dateAdded {
  DCHECK(node_);
  return [NSDate dateWithTimeIntervalSince1970:node_->date_added().ToDoubleT()];
}

- (void)setDateAdded:(NSDate*)date {
  DCHECK(node_);
  model_->SetDateAdded(node_,
                       base::Time::FromDoubleT([date timeIntervalSince1970]));
}

- (NSDate*)dateFolderModified {
  DCHECK(node_);
  return [NSDate
      dateWithTimeIntervalSince1970:node_->date_folder_modified().ToDoubleT()];
}

- (void)setDateFolderModified:(NSDate*)date {
  DCHECK(node_);
  model_->SetDateFolderModified(
      node_, base::Time::FromDoubleT([date timeIntervalSince1970]));
}

- (bool)isFolder {
  DCHECK(node_);
  return node_->is_folder();
}

- (bool)isUrl {
  DCHECK(node_);
  return node_->is_url();
}

- (bool)isFavIconLoaded {
  DCHECK(node_);
  return node_->is_favicon_loaded();
}

- (bool)isFavIconLoading {
  DCHECK(node_);
  return node_->is_favicon_loading();
}

- (bool)isVisible {
  DCHECK(node_);
  return node_->IsVisible();
}

- (bool)isValid {
  return node_ && model_;
}

- (bool)getMetaInfo:(NSString*)key value:(NSString**)value {
  DCHECK(node_);
  std::string value_;
  bool result = node_->GetMetaInfo(base::SysNSStringToUTF8(key), &value_);
  if (value) {
    *value = base::SysUTF8ToNSString(value_);
  }
  return result;
}

- (void)setMetaInfo:(NSString*)key value:(NSString*)value {
  DCHECK(node_);
  model_->SetNodeMetaInfo(node_, base::SysNSStringToUTF8(key),
                          base::SysNSStringToUTF8(value));
}

- (void)deleteMetaInfo:(NSString*)key {
  DCHECK(node_);
  return model_->DeleteNodeMetaInfo(node_, base::SysNSStringToUTF8(key));
}

- (NSString*)titleUrlNodeTitle {
  DCHECK(node_);
  return base::SysUTF16ToNSString(node_->GetTitledUrlNodeTitle());
}

- (NSURL*)titleUrlNodeUrl {
  DCHECK(node_);
  return net::NSURLWithGURL(node_->GetTitledUrlNodeUrl());
}

- (BookmarkNode*)parent {
  DCHECK(node_);
  const bookmarks::BookmarkNode* parent_ = node_->parent();
  if (parent_) {
    return [[BookmarkNode alloc] initWithNode:parent_ model:model_];
  }
  return nil;
}

- (NSArray<BookmarkNode*>*)children {
  DCHECK(node_);
  NSMutableArray* result = [[NSMutableArray alloc] init];
  for (const auto& child : node_->children()) {
    [result addObject:[[BookmarkNode alloc] initWithNode:child.get()
                                                   model:model_]];
  }
  return result;
}

- (NSArray<BookmarkNode*>*)nestedChildFolders {
  DCHECK(node_);
  auto find_folders_recursively = [](const bookmarks::BookmarkNode* node)
                                    -> std::vector<const bookmarks::BookmarkNode*> {
    auto find_impl = [](auto* node, auto& nodes, auto& find_ref) mutable -> void {
      for (const auto& child : node->children()) {
        if (child->is_folder()) {
          find_ref(child.get(), nodes, find_ref);
          nodes.push_back(child.get());
        }
      }
    };
    std::vector<const bookmarks::BookmarkNode*> nodes;
    find_impl(node, nodes, find_impl);
    return nodes;
  };
  
  NSMutableArray* result = [[NSMutableArray alloc] init];
  for (const bookmarks::BookmarkNode* child : find_folders_recursively(node_)) {
    [result addObject: [[BookmarkNode alloc] initWithNode:child model:model_]];
  }
  return result;
}

- (NSUInteger)childCount {
  DCHECK(node_);
  return node_->GetTotalNodeCount() - 1;
}

- (BookmarkNode*)childAtIndex:(NSUInteger)index {
  DCHECK(node_);
  const auto& children = node_->children();
  if (static_cast<std::size_t>(index) < children.size()) {
    return [[BookmarkNode alloc] initWithNode:children[index].get() model:model_];;
  }
  return nil;
}

- (BookmarkNode*)addChildFolderWithTitle:(NSString*)title {
  DCHECK(node_);
  if ([self isFolder]) {
    const bookmarks::BookmarkNode* node = model_->AddFolder(
        node_, node_->children().size(), base::SysNSStringToUTF16(title));
    return [[BookmarkNode alloc] initWithNode:node model:model_];
  }
  return nil;
}

- (BookmarkNode*)addChildBookmarkWithTitle:(NSString*)title url:(NSURL*)url {
  DCHECK(node_);
  if ([self isFolder]) {
    const bookmarks::BookmarkNode* node = model_->AddURL(
        node_, node_->children().size(), base::SysNSStringToUTF16(title),
        net::GURLWithNSURL(url));
    return [[BookmarkNode alloc] initWithNode:node model:model_];
  }
  return nil;
}

- (void)moveToParent:(BookmarkNode*)parent {
  DCHECK(node_);
  if ([parent isFolder]) {
    model_->Move(node_, parent->node_, parent->node_->children().size());
  }
}

- (void)moveToParent:(BookmarkNode*)parent index:(NSUInteger)index {
  DCHECK(node_);
  if ([parent isFolder]) {
    model_->Move(node_, parent->node_, index);
  }
}

- (NSInteger)indexOfChild:(BookmarkNode*)child {
  DCHECK(node_);
  return node_->GetIndexOf(child->node_);
}

- (void)remove {
  DCHECK(node_);
  model_->Remove(node_);
  node_ = nil;
  model_ = nil;
}

- (const bookmarks::BookmarkNode*)getNode {
  return self->node_;
}

@end

@interface BraveBookmarksAPI () {
  bookmarks::BookmarkModel* bookmarkModel_;   // NOT OWNED
  BookmarkUndoService* bookmarkUndoService_;  // NOT OWNED
}
@end

@implementation BraveBookmarksAPI
+ (instancetype)sharedBookmarksAPI {
  static BraveBookmarksAPI* instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[BraveBookmarksAPI alloc] init];
  });
  return instance;
}

- (instancetype)init {
  if ((self = [super init])) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    ios::ChromeBrowserStateManager* browserStateManager =
        GetApplicationContext()->GetChromeBrowserStateManager();
    ChromeBrowserState* browserState =
        browserStateManager->GetLastUsedBrowserState();
    bookmarkModel_ =
        ios::BookmarkModelFactory::GetForBrowserState(browserState);
    bookmarkUndoService_ =
        ios::BookmarkUndoServiceFactory::GetForBrowserState(browserState);
  }
  return self;
}

- (void)dealloc {
  bookmarkModel_ = nil;
  bookmarkUndoService_ = nil;
}

- (BookmarkNode*)rootNode {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  const bookmarks::BookmarkNode* node = bookmarkModel_->root_node();
  if (node) {
    return [[BookmarkNode alloc] initWithNode:node model:bookmarkModel_];
  }
  return nil;
}

- (BookmarkNode*)otherNode {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  const bookmarks::BookmarkNode* node = bookmarkModel_->other_node();
  if (node) {
    return [[BookmarkNode alloc] initWithNode:node model:bookmarkModel_];
  }
  return nil;
}

- (BookmarkNode*)mobileNode {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  const bookmarks::BookmarkNode* node = bookmarkModel_->mobile_node();
  if (node) {
    return [[BookmarkNode alloc] initWithNode:node model:bookmarkModel_];
  }
  return nil;
}

- (BookmarkNode*)desktopNode {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  const bookmarks::BookmarkNode* node = bookmarkModel_->bookmark_bar_node();
  if (node) {
    return [[BookmarkNode alloc] initWithNode:node model:bookmarkModel_];
  }
  return nil;
}

- (bool)isLoaded {
  return bookmarkModel_->loaded();
}

- (id<BookmarkModelListener>)addObserver:(id<BookmarkModelObserver>)observer {
  return [[BookmarkModelListenerImpl alloc] init:observer
                                   bookmarkModel:bookmarkModel_];
}

- (void)removeObserver:(id<BookmarkModelListener>)observer {
  [observer destroy];
}

- (bool)editingEnabled {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  ios::ChromeBrowserStateManager* browserStateManager =
      GetApplicationContext()->GetChromeBrowserStateManager();
  ChromeBrowserState* browserState =
      browserStateManager->GetLastUsedBrowserState();

  PrefService* prefs = user_prefs::UserPrefs::Get(browserState);
  return prefs->GetBoolean(bookmarks::prefs::kEditBookmarksEnabled);
}

- (BookmarkNode*)createFolderWithTitle:(NSString*)title {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  if ([self mobileNode]) {
    return [self createFolderWithParent:[self mobileNode] title:title];
  }
  return nil;
}

- (BookmarkNode*)createFolderWithParent:(BookmarkNode*)parent
                                  title:(NSString*)title {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(parent);
  const bookmarks::BookmarkNode* defaultFolder = [parent getNode];

  const bookmarks::BookmarkNode* new_node =
      bookmarkModel_->AddFolder(defaultFolder, defaultFolder->children().size(),
                                base::SysNSStringToUTF16(title));
  if (new_node) {
    return [[BookmarkNode alloc] initWithNode:new_node model:bookmarkModel_];
  }
  return nil;
}

- (BookmarkNode*)createBookmarkWithTitle:(NSString*)title url:(NSURL*)url {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  if ([self mobileNode]) {
    return [self createBookmarkWithParent:[self mobileNode]
                                    title:title
                                  withUrl:url];
  }
  return nil;
}

- (BookmarkNode*)createBookmarkWithParent:(BookmarkNode*)parent
                                    title:(NSString*)title
                                  withUrl:(NSURL*)url {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(parent);
  const bookmarks::BookmarkNode* defaultFolder = [parent getNode];

  const bookmarks::BookmarkNode* new_node = bookmarkModel_->AddURL(
      defaultFolder, defaultFolder->children().size(),
      base::SysNSStringToUTF16(title), net::GURLWithNSURL(url));
  if (new_node) {
    return [[BookmarkNode alloc] initWithNode:new_node model:bookmarkModel_];
  }
  return nil;
}

- (void)removeBookmark:(BookmarkNode*)bookmark {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  [bookmark remove];
}

- (void)removeAll {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  bookmarkModel_->RemoveAllUserBookmarks();
}

- (NSArray<BookmarkNode*>*)searchWithQuery:(NSString*)query
                                  maxCount:(NSUInteger)maxCount {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(bookmarkModel_->loaded());
  bookmarks::QueryFields queryFields;
  queryFields.word_phrase_query.reset(
      new base::string16(base::SysNSStringToUTF16(query)));
  std::vector<const bookmarks::BookmarkNode*> results;
  GetBookmarksMatchingProperties(bookmarkModel_, queryFields, maxCount,
                                 &results);

  NSMutableArray<BookmarkNode*>* nodes = [[NSMutableArray alloc] init];
  for (const bookmarks::BookmarkNode* bookmark : results) {
    BookmarkNode* node = [[BookmarkNode alloc] initWithNode:bookmark
                                                      model:bookmarkModel_];
    [nodes addObject:node];
  }
  return nodes;
}

- (void)undo {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  bookmarkUndoService_->undo_manager()->Undo();
}

- (bookmarks::BookmarkModel*)getModel {
  return bookmarkModel_;
}
@end
