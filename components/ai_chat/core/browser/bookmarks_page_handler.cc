// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/bookmarks_page_handler.h"

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/strings/utf_string_conversions.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"

namespace ai_chat {

namespace {

mojom::BookmarkPtr ToMojoBookmark(const bookmarks::BookmarkNode* node) {
  CHECK(!node->is_folder()) << "Folders are not supported!";

  return mojom::Bookmark::New(node->id(), base::UTF16ToUTF8(node->GetTitle()),
                              node->url());
}

}  // namespace

BookmarksPageHandler::BookmarksPageHandler(
    bookmarks::BookmarkModel* bookmark_model,
    mojo::PendingReceiver<mojom::BookmarksPageHandler> receiver)
    : bookmark_model_(bookmark_model) {
  CHECK(bookmark_model_);
  receiver_.Bind(std::move(receiver));
}

BookmarksPageHandler::~BookmarksPageHandler() = default;

void BookmarksPageHandler::GetBookmarks(GetBookmarksCallback callback) {
  std::move(callback).Run(GetAllBookmarks());
}

std::vector<mojom::BookmarkPtr> BookmarksPageHandler::GetAllBookmarks() {
  std::vector<mojom::BookmarkPtr> bookmarks;
  if (!bookmark_model_) {
    return bookmarks;
  }

  // Do a depth-first traversal of the bookmark tree extracting all non-folder
  // nodes.
  std::vector<const bookmarks::BookmarkNode*> frontier;
  frontier.push_back(bookmark_model_->root_node());

  while (!frontier.empty()) {
    auto* node = frontier.back();
    frontier.pop_back();

    if (node->is_folder()) {
      auto& children = node->children();
      std::ranges::transform(children.begin(), children.end(),
                             std::back_inserter(frontier),
                             [](const auto& c) { return c.get(); });
    } else {
      bookmarks.push_back(ToMojoBookmark(node));
    }
  }

  return bookmarks;
}

}  // namespace ai_chat
