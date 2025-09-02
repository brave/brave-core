// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_BOOKMARKS_SERVICE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_BOOKMARKS_SERVICE_H_

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/ai_chat/core/common/mojom/bookmarks.mojom.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_model_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace bookmarks {
class BookmarkModel;
}

namespace ai_chat {

class BookmarksService : public mojom::BookmarksService,
                         public bookmarks::BookmarkModelObserver {
 public:
  BookmarksService(bookmarks::BookmarkModel* bookmark_model,
                   mojo::PendingReceiver<mojom::BookmarksService> receiver);
  ~BookmarksService() override;

  BookmarksService(const BookmarksService&) = delete;
  BookmarksService& operator=(const BookmarksService&) = delete;

  // mojom::BookmarksService
  void AddListener(
      mojo::PendingRemote<mojom::BookmarksListener> listener) override;

  // bookmarks::BookmarkModelObserver:
  void BookmarkModelLoaded(bool ids_reassigned) override;
  void BookmarkNodeAdded(const bookmarks::BookmarkNode* parent,
                         size_t index,
                         bool added_by_user) override;
  void BookmarkNodeRemoved(const bookmarks::BookmarkNode* parent,
                           size_t old_index,
                           const bookmarks::BookmarkNode* node,
                           const std::set<GURL>& no_longer_bookmarked,
                           const base::Location& location) override;
  void BookmarkNodeMoved(const bookmarks::BookmarkNode* old_parent,
                         size_t old_index,
                         const bookmarks::BookmarkNode* new_parent,
                         size_t new_index) override {}
  void BookmarkNodeChanged(const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeFaviconChanged(
      const bookmarks::BookmarkNode* node) override {}
  void BookmarkNodeChildrenReordered(
      const bookmarks::BookmarkNode* node) override {}
  void BookmarkAllUserNodesRemoved(const std::set<GURL>& removed_urls,
                                   const base::Location& location) override;

 private:
  mojom::BookmarksChangePtr GetAllBookmarks();

  raw_ptr<bookmarks::BookmarkModel> bookmark_model_;

  base::ScopedObservation<bookmarks::BookmarkModel,
                          bookmarks::BookmarkModelObserver>
      bookmark_model_observation_{this};

  mojo::Receiver<ai_chat::mojom::BookmarksService> receiver_{this};
  mojo::RemoteSet<mojom::BookmarksListener> bookmark_listeners_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_BOOKMARKS_SERVICE_H_
