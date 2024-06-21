/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/bookmarks/brave_bookmarks_api.h"

#include "base/compiler_specific.h"
#include "base/containers/adapters.h"
#include "base/containers/stack.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/uuid.h"
#include "brave/ios/browser/api/bookmarks/bookmark_model_listener_ios.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/browser/bookmark_uuids.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/undo/bookmark_undo_service.h"
#include "components/undo/undo_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "ios/chrome/browser/bookmarks/model/legacy_bookmark_model.h"
#include "ios/chrome/browser/bookmarks/ui_bundled/bookmark_utils_ios.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state_manager.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#import "net/base/apple/url_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Used from the iOS/Swift side only..
@implementation BookmarkFolder
- (instancetype)initWithNode:(IOSBookmarkNode*)node
            indentationLevel:(NSInteger)indentationLevel {
  if ((self = [super init])) {
    _bookmarkNode = node;
    _indentationLevel = indentationLevel;
  }
  return self;
}
@end

@interface IOSBookmarkNode () {
  const bookmarks::BookmarkNode* node_;
  bookmarks::BookmarkModel* model_;  // UNOWNED
  bool owned_;
}
@end

@implementation IOSBookmarkNode

- (instancetype)initWithNode:(const bookmarks::BookmarkNode*)node
                       model:(bookmarks::BookmarkModel*)model {
  if ((self = [super init])) {
    node_ = node;
    model_ = model;
    owned_ = false;
  }
  return self;
}

- (instancetype)initWithTitle:(NSString*)title
                           id:(int64_t)id
                         guid:(NSString*)guid
                          url:(NSURL*)url
                    dateAdded:(NSDate*)dateAdded
                 dateModified:(NSDate*)dateModified
                     children:(NSArray<IOSBookmarkNode*>*)children {
  if ((self = [super init])) {
    base::Uuid guid_ = base::Uuid();
    int64_t id_ = static_cast<int64_t>(id);

    if ([guid length] > 0) {
      std::u16string guid_string = base::SysNSStringToUTF16(guid);
      guid_ = base::Uuid::ParseCaseInsensitive(guid_string);
      DCHECK(guid_.is_valid());
    } else {
      guid_ = base::Uuid::GenerateRandomV4();
    }

    GURL gurl_ = net::GURLWithNSURL(url);
    bookmarks::BookmarkNode* node =
        new bookmarks::BookmarkNode(id_, guid_, gurl_);
    node->SetTitle(base::SysNSStringToUTF16(title));

    if (dateAdded) {
      node->set_date_added(base::Time::FromSecondsSinceUnixEpoch(
          [dateAdded timeIntervalSince1970]));
    }

    if (dateModified) {
      node->set_date_folder_modified(base::Time::FromSecondsSinceUnixEpoch(
          [dateModified timeIntervalSince1970]));
    }

    for (IOSBookmarkNode* child : children) {
      DCHECK(child->owned_);  // Bookmark must be owned and non-const. IE:
                              // Allocated from iOS/Swift.
      child->owned_ = false;
      child->node_ = node->Add(std::unique_ptr<bookmarks::BookmarkNode>(
          const_cast<bookmarks::BookmarkNode*>(child->node_)));
    }

    node_ = node;
    model_ = nil;
    owned_ = true;
  }
  return self;
}

- (void)dealloc {
  if (owned_) {
    // All Objective-C++ class pointers are reference counted.
    // No need for copy or move constructor & assignment operators.
    // They're automatic RAII.. So we can safely delete an owned raw pointer
    // here.
    delete node_;
    node_ = nullptr;
    owned_ = false;
  }
  node_ = nullptr;
  model_ = nullptr;
}

+ (NSString*)rootNodeGuid {
  return base::SysUTF8ToNSString(bookmarks::kRootNodeUuid);
}

+ (NSString*)bookmarkBarNodeGuid {
  return base::SysUTF8ToNSString(bookmarks::kBookmarkBarNodeUuid);
}

+ (NSString*)otherBookmarksNodeGuid {
  return base::SysUTF8ToNSString(bookmarks::kOtherBookmarksNodeUuid);
}

+ (NSString*)mobileBookmarksNodeGuid {
  return base::SysUTF8ToNSString(bookmarks::kMobileBookmarksNodeUuid);
}

+ (NSString*)managedNodeGuid {
  return base::SysUTF8ToNSString(bookmarks::kManagedNodeUuid);
}

- (bool)isPermanentNode {
  DCHECK(node_);
  return node_->is_permanent_node();
}

- (void)setTitle:(NSString*)title {
  DCHECK(node_);
  DCHECK(model_);
  model_->SetTitle(node_, base::SysNSStringToUTF16(title),
                   bookmarks::metrics::BookmarkEditSource::kUser);
}

- (NSUInteger)nodeId {
  DCHECK(node_);
  return node_->id();
}

- (NSString*)guid {
  DCHECK(node_);
  return base::SysUTF8ToNSString(node_->uuid().AsLowercaseString());
}

- (NSURL*)url {
  DCHECK(node_);
  return net::NSURLWithGURL(node_->url());
}

- (void)setUrl:(NSURL*)url {
  DCHECK(node_);
  DCHECK(model_);
  model_->SetURL(node_, net::GURLWithNSURL(url),
                 bookmarks::metrics::BookmarkEditSource::kUser);
}

- (NSURL*)iconUrl {
  DCHECK(node_);
  const GURL* url = node_->icon_url();
  return url ? net::NSURLWithGURL(*url) : nullptr;
}

- (UIImage*)icon {
  DCHECK(node_);
  DCHECK(model_);
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
  return [NSDate dateWithTimeIntervalSince1970:node_->date_added()
                                                   .InSecondsFSinceUnixEpoch()];
}

- (void)setDateAdded:(NSDate*)date {
  DCHECK(node_);
  DCHECK(model_);
  model_->SetDateAdded(node_, base::Time::FromSecondsSinceUnixEpoch(
                                  [date timeIntervalSince1970]));
}

- (NSDate*)dateFolderModified {
  DCHECK(node_);
  return [NSDate dateWithTimeIntervalSince1970:node_->date_folder_modified()
                                                   .InSecondsFSinceUnixEpoch()];
}

- (void)setDateFolderModified:(NSDate*)date {
  DCHECK(node_);
  DCHECK(model_);
  model_->SetDateFolderModified(node_, base::Time::FromSecondsSinceUnixEpoch(
                                           [date timeIntervalSince1970]));
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
  DCHECK(model_);
  model_->SetNodeMetaInfo(node_, base::SysNSStringToUTF8(key),
                          base::SysNSStringToUTF8(value));
}

- (void)deleteMetaInfo:(NSString*)key {
  DCHECK(node_);
  DCHECK(model_);
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

- (IOSBookmarkNode*)parent {
  DCHECK(node_);
  const bookmarks::BookmarkNode* parent_ = node_->parent();
  if (parent_) {
    return [[IOSBookmarkNode alloc] initWithNode:parent_ model:model_];
  }
  return nil;
}

- (NSArray<IOSBookmarkNode*>*)children {
  DCHECK(node_);
  NSMutableArray* result = [[NSMutableArray alloc] init];
  for (const auto& child : node_->children()) {
    [result addObject:[[IOSBookmarkNode alloc] initWithNode:child.get()
                                                      model:model_]];
  }
  return result;
}

- (NSArray<BookmarkFolder*>*)nestedChildFolders {
  // Returns a list of ALL nested folders
  return [self nestedChildFoldersFiltered:^BOOL(BookmarkFolder*){ return true; }];
}

- (NSUInteger)childCount {
  DCHECK(node_);
  return node_->children().size();
}

- (NSUInteger)totalCount {
  DCHECK(node_);
  return node_->GetTotalNodeCount() - 1;
}

- (IOSBookmarkNode*)childAtIndex:(NSUInteger)index {
  DCHECK(node_);
  const auto& children = node_->children();
  if (static_cast<std::size_t>(index) < children.size()) {
    return [[IOSBookmarkNode alloc] initWithNode:children[index].get()
                                           model:model_];
    ;
  }
  return nil;
}

// Retrieves a list of nested child folders filtered by the predicate |included|
// iOS calls it like: `self.nestedChildFolders(where: { return some_condition })`
- (NSArray<BookmarkFolder*>*)nestedChildFoldersFiltered:(BOOL(^)(BookmarkFolder*))included {
  DCHECK(node_);

  std::vector<const bookmarks::BookmarkNode*> bookmarks = {node_};

  base::stack<std::pair<const bookmarks::BookmarkNode*, std::int32_t>> stack;
  for (const bookmarks::BookmarkNode* bookmark : base::Reversed(bookmarks)) {
    stack.emplace(bookmark, 0);
  }

  NSMutableArray* result = [[NSMutableArray alloc] init];
  while (!stack.empty()) {
    const bookmarks::BookmarkNode* node = stack.top().first;
    std::int32_t depth = stack.top().second;
    stack.pop();

    IOSBookmarkNode* ios_bookmark_node =
        [[IOSBookmarkNode alloc] initWithNode:node model:model_];
    BookmarkFolder* ios_bookmark_folder = [[BookmarkFolder alloc] initWithNode:ios_bookmark_node
                                                              indentationLevel:depth];

    if (included(ios_bookmark_folder)) {
      // Store the folder + its depth
      [result addObject:ios_bookmark_folder];

      bookmarks.clear();
      for (const auto& child : node->children()) {
        if (child->is_folder()) {
          ios_bookmark_node = [[IOSBookmarkNode alloc] initWithNode:child.get() model:model_];
          ios_bookmark_folder = [[BookmarkFolder alloc] initWithNode:ios_bookmark_node
                                                    indentationLevel:depth + 1];
          if (included(ios_bookmark_folder)) {
            bookmarks.push_back(child.get());
          }
        }
      }
    }

    for (const auto* bookmark : base::Reversed(bookmarks)) {
      stack.emplace(bookmark, depth + 1);
    }
  }

  return result;
}

- (IOSBookmarkNode*)addChildFolderWithTitle:(NSString*)title {
  DCHECK(node_);
  DCHECK(model_);
  if ([self isFolder]) {
    const bookmarks::BookmarkNode* node = model_->AddFolder(
        node_, node_->children().size(), base::SysNSStringToUTF16(title));
    return [[IOSBookmarkNode alloc] initWithNode:node model:model_];
  }
  return nil;
}

- (IOSBookmarkNode*)addChildBookmarkWithTitle:(NSString*)title url:(NSURL*)url {
  DCHECK(node_);
  DCHECK(model_);
  if ([self isFolder]) {
    const bookmarks::BookmarkNode* node = model_->AddURL(
        node_, node_->children().size(), base::SysNSStringToUTF16(title),
        net::GURLWithNSURL(url));
    return [[IOSBookmarkNode alloc] initWithNode:node model:model_];
  }
  return nil;
}

- (void)moveToParent:(IOSBookmarkNode*)parent {
  DCHECK(node_);
  DCHECK(model_);
  if ([parent isFolder]) {
    model_->Move(node_, parent->node_, parent->node_->children().size());
  }
}

- (void)moveToParent:(IOSBookmarkNode*)parent index:(NSUInteger)index {
  DCHECK(node_);
  DCHECK(model_);
  if ([parent isFolder]) {
    model_->Move(node_, parent->node_, index);
  }
}

- (NSInteger)indexOfChild:(IOSBookmarkNode*)child {
  DCHECK(node_);
  auto index = node_->GetIndexOf(child->node_);
  if (!index)
    return NSNotFound;

  return base::checked_cast<NSInteger>(*index);
}

- (bool)hasAncestor:(IOSBookmarkNode*)parent {
  DCHECK(node_);
  return node_->HasAncestor(parent->node_);
}

- (void)remove {
  DCHECK(node_);
  DCHECK(model_);
  model_->Remove(node_, bookmarks::metrics::BookmarkEditSource::kOther,
                 FROM_HERE);
  node_ = nil;
  model_ = nil;
}

- (const bookmarks::BookmarkNode*)getNode {
  return node_;
}

- (void)setNativeParent:(bookmarks::BookmarkNode*)parent {
  DCHECK(owned_);
  owned_ = false;
  node_ = parent->Add(std::unique_ptr<bookmarks::BookmarkNode>(
      const_cast<bookmarks::BookmarkNode*>(node_)));
}

@end

@interface BraveBookmarksAPI () {
  bookmarks::BookmarkModel* bookmark_model_;    // NOT OWNED
  BookmarkUndoService* bookmark_undo_service_;  // NOT OWNED
}
@end

@implementation BraveBookmarksAPI

- (instancetype)initWithBookmarkModel:(bookmarks::BookmarkModel*)bookmarkModel
                  bookmarkUndoService:
                      (BookmarkUndoService*)bookmarkUndoService {
  if ((self = [super init])) {
    bookmark_model_ = bookmarkModel;
    bookmark_undo_service_ = bookmarkUndoService;
  }
  return self;
}

- (void)dealloc {
  bookmark_model_ = nil;
  bookmark_undo_service_ = nil;
}

- (IOSBookmarkNode*)rootNode {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  const bookmarks::BookmarkNode* node = bookmark_model_->root_node();
  if (node) {
    return [[IOSBookmarkNode alloc] initWithNode:node model:bookmark_model_];
  }
  return nil;
}

- (IOSBookmarkNode*)otherNode {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  const bookmarks::BookmarkNode* node = bookmark_model_->other_node();
  if (node) {
    return [[IOSBookmarkNode alloc] initWithNode:node model:bookmark_model_];
  }
  return nil;
}

- (IOSBookmarkNode*)mobileNode {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  const bookmarks::BookmarkNode* node = bookmark_model_->mobile_node();
  if (node) {
    return [[IOSBookmarkNode alloc] initWithNode:node model:bookmark_model_];
  }
  return nil;
}

- (IOSBookmarkNode*)desktopNode {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  const bookmarks::BookmarkNode* node = bookmark_model_->bookmark_bar_node();
  if (node) {
    return [[IOSBookmarkNode alloc] initWithNode:node model:bookmark_model_];
  }
  return nil;
}

- (bool)isLoaded {
  return bookmark_model_->loaded();
}

- (id<BookmarkModelListener>)addObserver:(id<BookmarkModelObserver>)observer {
  return [[BookmarkModelListenerImpl alloc] init:observer
                                   bookmarkModel:bookmark_model_];
}

- (void)removeObserver:(id<BookmarkModelListener>)observer {
  [observer destroy];
}

- (bool)editingEnabled {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  ios::ChromeBrowserStateManager* browserStateManager =
      GetApplicationContext()->GetChromeBrowserStateManager();
  ChromeBrowserState* browserState =
      browserStateManager->GetLastUsedBrowserStateDeprecatedDoNotUse();

  PrefService* prefs = user_prefs::UserPrefs::Get(browserState);
  return prefs->GetBoolean(bookmarks::prefs::kEditBookmarksEnabled);
}

- (IOSBookmarkNode*)createFolderWithTitle:(NSString*)title {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  if ([self mobileNode]) {
    return [self createFolderWithParent:[self mobileNode] title:title];
  }
  return nil;
}

- (IOSBookmarkNode*)createFolderWithParent:(IOSBookmarkNode*)parent
                                     title:(NSString*)title {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(parent);
  const bookmarks::BookmarkNode* defaultFolder = [parent getNode];

  const bookmarks::BookmarkNode* new_node = bookmark_model_->AddFolder(
      defaultFolder, defaultFolder->children().size(),
      base::SysNSStringToUTF16(title));
  if (new_node) {
    return [[IOSBookmarkNode alloc] initWithNode:new_node
                                           model:bookmark_model_];
  }
  return nil;
}

- (IOSBookmarkNode*)createBookmarkWithTitle:(NSString*)title url:(NSURL*)url {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  if ([self mobileNode]) {
    return [self createBookmarkWithParent:[self mobileNode]
                                    title:title
                                  withUrl:url];
  }
  return nil;
}

- (IOSBookmarkNode*)createBookmarkWithParent:(IOSBookmarkNode*)parent
                                       title:(NSString*)title
                                     withUrl:(NSURL*)url {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(parent);
  const bookmarks::BookmarkNode* defaultFolder = [parent getNode];

  const bookmarks::BookmarkNode* new_node = bookmark_model_->AddURL(
      defaultFolder, defaultFolder->children().size(),
      base::SysNSStringToUTF16(title), net::GURLWithNSURL(url));
  if (new_node) {
    return [[IOSBookmarkNode alloc] initWithNode:new_node
                                           model:bookmark_model_];
  }
  return nil;
}

- (IOSBookmarkNode*)getNodeById:(NSInteger)nodeId {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  const bookmarks::BookmarkNode* node = bookmarks::GetBookmarkNodeByID(
      bookmark_model_, static_cast<std::int64_t>(nodeId));
  if (node) {
    return [[IOSBookmarkNode alloc] initWithNode:node model:bookmark_model_];
  }
  return nil;
}

- (void)removeBookmark:(IOSBookmarkNode*)bookmark {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  [bookmark remove];
}

- (void)removeAll {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  bookmark_model_->RemoveAllUserBookmarks(FROM_HERE);
}

- (void)searchWithQuery:(NSString*)queryArg
               maxCount:(NSUInteger)maxCountArg
             completion:(void (^)(NSArray<IOSBookmarkNode*>*))callback {
  __weak BraveBookmarksAPI* weak_bookmarks_api = self;
  auto search_with_query =
      ^(NSString* query, NSUInteger maxCount,
        void (^completion)(NSArray<IOSBookmarkNode*>*)) {
        BraveBookmarksAPI* bookmarks_api = weak_bookmarks_api;
        if (!bookmarks_api) {
          completion(@[]);
          return;
        }

        DCHECK_CURRENTLY_ON(web::WebThread::UI);
        DCHECK(bookmarks_api->bookmark_model_->loaded());

        bookmarks::QueryFields queryFields;
        queryFields.word_phrase_query.reset(
            new std::u16string(base::SysNSStringToUTF16(query)));
        std::vector<const bookmarks::BookmarkNode*> results =
            GetBookmarksMatchingProperties(bookmarks_api->bookmark_model_,
                                           queryFields, maxCount);

        NSMutableArray<IOSBookmarkNode*>* nodes = [[NSMutableArray alloc] init];
        for (const bookmarks::BookmarkNode* bookmark : results) {
          IOSBookmarkNode* node = [[IOSBookmarkNode alloc]
              initWithNode:bookmark
                     model:bookmarks_api->bookmark_model_];
          [nodes addObject:node];
        }
        completion(nodes);
      };

  web::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(search_with_query, queryArg, maxCountArg, callback));
}

- (void)undo {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  bookmark_undo_service_->undo_manager()->Undo();
}

- (bookmarks::BookmarkModel*)getModel {
  return bookmark_model_;
}
@end
