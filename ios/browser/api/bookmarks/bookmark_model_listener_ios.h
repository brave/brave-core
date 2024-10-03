/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BOOKMARKS_BOOKMARK_MODEL_LISTENER_IOS_H_
#define BRAVE_IOS_BROWSER_API_BOOKMARKS_BOOKMARK_MODEL_LISTENER_IOS_H_

#import <Foundation/Foundation.h>

#include <set>

#include "base/memory/raw_ptr.h"
#include "brave/ios/browser/api/bookmarks/brave_bookmarks_observer.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_model_observer.h"

@interface BookmarkModelListenerImpl : NSObject <BookmarkModelListener>
- (instancetype)init:(id<BookmarkModelObserver>)observer
       bookmarkModel:(void*)bookmarkModel;
@end

namespace brave {
namespace ios {
class BookmarkModelListener : public bookmarks::BookmarkModelObserver {
 public:
  explicit BookmarkModelListener(id<BookmarkModelObserver> observer,
                                 bookmarks::BookmarkModel* model);
  ~BookmarkModelListener() override;

 private:
  // BookmarksModelListener implementation
  void BookmarkModelLoaded(bool ids_reassigned) override;
  void BookmarkModelBeingDeleted() override;
  void BookmarkNodeMoved(const bookmarks::BookmarkNode* old_parent,
                         size_t old_index,
                         const bookmarks::BookmarkNode* new_parent,
                         size_t new_index) override;
  void BookmarkNodeAdded(const bookmarks::BookmarkNode* parent,
                         size_t index,
                         bool added_by_user) override;
  void BookmarkNodeRemoved(const bookmarks::BookmarkNode* parent,
                           size_t old_index,
                           const bookmarks::BookmarkNode* node,
                           const std::set<GURL>& removed_urls,
                           const base::Location& location) override;
  void BookmarkNodeChanged(const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeFaviconChanged(const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeChildrenReordered(
      const bookmarks::BookmarkNode* node) override;
  void BookmarkAllUserNodesRemoved(const std::set<GURL>& removed_urls,
                                   const base::Location& location) override;

  __strong id<BookmarkModelObserver> observer_;
  raw_ptr<bookmarks::BookmarkModel> model_;
};

}  // namespace ios
}  // namespace brave

#endif  // BRAVE_IOS_BROWSER_API_BOOKMARKS_BOOKMARK_MODEL_LISTENER_IOS_H_
