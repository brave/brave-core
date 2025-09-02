// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/bookmarks_service.h"

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

BookmarksService::BookmarksService(
    bookmarks::BookmarkModel* bookmark_model,
    mojo::PendingReceiver<mojom::BookmarksService> receiver)
    : bookmark_model_(bookmark_model) {
  CHECK(bookmark_model_);
  bookmark_model_observation_.Observe(bookmark_model_);

  receiver_.Bind(std::move(receiver));
}

BookmarksService::~BookmarksService() = default;

void BookmarksService::AddListener(
    mojo::PendingRemote<mojom::BookmarksListener> pending_listener) {
  auto id = bookmark_listeners_.Add(std::move(pending_listener));
  auto* listener = bookmark_listeners_.Get(id);
  if (listener) {
    listener->Changed(GetAllBookmarks());
  }
}

void BookmarksService::BookmarkModelLoaded(bool ids_reassigned) {
  auto change = GetAllBookmarks();
  for (const auto& listener : bookmark_listeners_) {
    listener->Changed(change->Clone());
  }
}

void BookmarksService::BookmarkNodeAdded(const bookmarks::BookmarkNode* parent,
                                         size_t index,
                                         bool added_by_user) {
  if (!parent || index >= parent->children().size()) {
    return;
  }

  auto* node = parent->children()[index].get();
  if (!node || node->is_folder()) {
    return;
  }

  auto change = mojom::BookmarksChange::New();
  change->addedOrUpdated[node->url().spec()] = ToMojoBookmark(node);

  for (const auto& listener : bookmark_listeners_) {
    listener->Changed(change->Clone());
  }
}

void BookmarksService::BookmarkNodeRemoved(
    const bookmarks::BookmarkNode* parent,
    size_t old_index,
    const bookmarks::BookmarkNode* node,
    const std::set<GURL>& no_longer_bookmarked,
    const base::Location& location) {
  if (node->is_folder()) {
    return;
  }

  auto change = mojom::BookmarksChange::New();
  change->removed.push_back(node->url().spec());

  for (const auto& listener : bookmark_listeners_) {
    listener->Changed(change->Clone());
  }
}

void BookmarksService::BookmarkNodeChanged(
    const bookmarks::BookmarkNode* node) {
  if (node->is_folder()) {
    return;
  }
  auto change = mojom::BookmarksChange::New();
  change->addedOrUpdated[node->url().spec()] = ToMojoBookmark(node);

  for (const auto& listener : bookmark_listeners_) {
    listener->Changed(std::move(change));
  }
}

mojom::BookmarksChangePtr BookmarksService::GetAllBookmarks() {
  auto change = mojom::BookmarksChange::New();
  if (!bookmark_model_) {
    return change;
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
      change->addedOrUpdated[node->url().spec()] = ToMojoBookmark(node);
    }
  }

  return change;
}

void BookmarksService::BookmarkAllUserNodesRemoved(
    const std::set<GURL>& removed_urls,
    const base::Location& location) {
  // Refresh all bookmarks since all user nodes were removed
  auto change = GetAllBookmarks();
  std::ranges::transform(removed_urls, std::back_inserter(change->removed),
                         [](const auto& url) { return url.spec(); });

  for (const auto& listener : bookmark_listeners_) {
    listener->Changed(change->Clone());
  }
}

}  // namespace ai_chat
