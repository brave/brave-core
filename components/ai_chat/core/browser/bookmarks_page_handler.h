// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_BOOKMARKS_PAGE_HANDLER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_BOOKMARKS_PAGE_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/ai_chat/core/common/mojom/bookmarks.mojom.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace bookmarks {
class BookmarkModel;
}

namespace ai_chat {

class BookmarksPageHandler : public mojom::BookmarksPageHandler {
 public:
  BookmarksPageHandler(
      bookmarks::BookmarkModel* bookmark_model,
      mojo::PendingReceiver<mojom::BookmarksPageHandler> receiver);
  ~BookmarksPageHandler() override;

  BookmarksPageHandler(const BookmarksPageHandler&) = delete;
  BookmarksPageHandler& operator=(const BookmarksPageHandler&) = delete;

  // mojom::BookmarksPageHandler
  void GetBookmarks(GetBookmarksCallback callback) override;

 private:
  std::vector<mojom::BookmarkPtr> GetAllBookmarks();

  raw_ptr<bookmarks::BookmarkModel> bookmark_model_;
  mojo::Receiver<ai_chat::mojom::BookmarksPageHandler> receiver_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_BOOKMARKS_PAGE_HANDLER_H_
