/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/bookmarks/bookmark_model_listener_ios.h"

#include <memory>

#include "base/check.h"
#include "base/compiler_specific.h"
#include "base/notreached.h"
#include "brave/ios/browser/api/bookmarks/brave_bookmarks_api.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_model_observer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface IOSBookmarkNode (Private)
- (instancetype)initWithNode:(const bookmarks::BookmarkNode*)node
                       model:(bookmarks::BookmarkModel*)model;
@end

namespace brave {
namespace ios {

BookmarkModelListener::BookmarkModelListener(id<BookmarkModelObserver> observer,
                                             bookmarks::BookmarkModel* model)
    : observer_(observer), model_(model) {
  DCHECK(observer_);
  DCHECK(model_);
  model_->AddObserver(this);
}

BookmarkModelListener::~BookmarkModelListener() {
  DCHECK(model_);
  model_->RemoveObserver(this);
}

void BookmarkModelListener::BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                                                bool ids_reassigned) {
  if ([observer_ respondsToSelector:@selector(bookmarkModelLoaded)]) {
    [observer_ bookmarkModelLoaded];
  }
}

void BookmarkModelListener::BookmarkModelBeingDeleted(
    bookmarks::BookmarkModel* model) {
  // This is an inconsistent state in the application lifecycle. The bookmark
  // model shouldn't disappear.
  NOTREACHED();
}

void BookmarkModelListener::BookmarkNodeMoved(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* old_parent,
    size_t old_index,
    const bookmarks::BookmarkNode* new_parent,
    size_t new_index) {
  if ([observer_ respondsToSelector:@selector(bookmarkNode:
                                           movedFromParent:toParent:)]) {
    const bookmarks::BookmarkNode* node =
        new_parent->children()[new_index].get();

    IOSBookmarkNode* ios_node = [[IOSBookmarkNode alloc] initWithNode:node
                                                                model:model];
    IOSBookmarkNode* ios_old_parent =
        [[IOSBookmarkNode alloc] initWithNode:old_parent model:model];
    IOSBookmarkNode* ios_new_parent =
        [[IOSBookmarkNode alloc] initWithNode:new_parent model:model];

    [observer_ bookmarkNode:ios_node
            movedFromParent:ios_old_parent
                   toParent:ios_new_parent];
  }
}

void BookmarkModelListener::BookmarkNodeAdded(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* parent,
    size_t index,
    bool added_by_user) {
  if ([observer_ respondsToSelector:@selector(bookmarkNodeChildrenChanged:)]) {
    IOSBookmarkNode* ios_parent = [[IOSBookmarkNode alloc] initWithNode:parent
                                                                  model:model];
    [observer_ bookmarkNodeChildrenChanged:ios_parent];
  }
}

void BookmarkModelListener::BookmarkNodeRemoved(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* parent,
    size_t old_index,
    const bookmarks::BookmarkNode* node,
    const std::set<GURL>& removed_urls) {
  IOSBookmarkNode* ios_node = [[IOSBookmarkNode alloc] initWithNode:node
                                                              model:model];
  IOSBookmarkNode* ios_parent = [[IOSBookmarkNode alloc] initWithNode:parent
                                                                model:model];

  if ([observer_ respondsToSelector:@selector(bookmarkNodeDeleted:
                                                       fromFolder:)]) {
    [observer_ bookmarkNodeDeleted:ios_node fromFolder:ios_parent];
  }

  if ([observer_ respondsToSelector:@selector(bookmarkNodeChildrenChanged:)]) {
    [observer_ bookmarkNodeChildrenChanged:ios_parent];
  }
}

void BookmarkModelListener::BookmarkNodeChanged(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {
  if ([observer_ respondsToSelector:@selector(bookmarkNodeChanged:)]) {
    IOSBookmarkNode* ios_node = [[IOSBookmarkNode alloc] initWithNode:node
                                                                model:model];
    [observer_ bookmarkNodeChanged:ios_node];
  }
}

void BookmarkModelListener::BookmarkNodeFaviconChanged(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {
  if ([observer_ respondsToSelector:@selector(bookmarkNodeFaviconChanged:)]) {
    IOSBookmarkNode* ios_node = [[IOSBookmarkNode alloc] initWithNode:node
                                                                model:model];
    [observer_ bookmarkNodeFaviconChanged:ios_node];
  }
}

void BookmarkModelListener::BookmarkNodeChildrenReordered(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {
  if ([observer_ respondsToSelector:@selector(bookmarkNodeChildrenChanged:)]) {
    IOSBookmarkNode* ios_node = [[IOSBookmarkNode alloc] initWithNode:node
                                                                model:model];
    [observer_ bookmarkNodeChildrenChanged:ios_node];
  }
}

void BookmarkModelListener::BookmarkAllUserNodesRemoved(
    bookmarks::BookmarkModel* model,
    const std::set<GURL>& removed_urls) {
  if ([observer_ respondsToSelector:@selector(bookmarkModelRemovedAllNodes)]) {
    [observer_ bookmarkModelRemovedAllNodes];
  }
}
}  // namespace ios
}  // namespace brave

@interface BookmarkModelListenerImpl () {
  std::unique_ptr<brave::ios::BookmarkModelListener> observer_;
  bookmarks::BookmarkModel* bookmarkModel_;
}
@end

@implementation BookmarkModelListenerImpl
- (instancetype)init:(id<BookmarkModelObserver>)observer
       bookmarkModel:(void*)model {
  if ((self = [super init])) {
    observer_ = std::make_unique<brave::ios::BookmarkModelListener>(
        observer, static_cast<bookmarks::BookmarkModel*>(model));
    bookmarkModel_ = static_cast<bookmarks::BookmarkModel*>(model);
  }
  return self;
}

- (void)dealloc {
  [self destroy];
}

- (void)destroy {
  observer_.reset();
}
@end
