/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/vendor/brave-ios/components/bookmarks/brave_bookmark_client.h"

#include "base/logging.h"
#include "base/metrics/user_metrics.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/browser/bookmark_storage.h"
#include "components/favicon_base/favicon_types.h"
#include "components/sync_bookmarks/bookmark_sync_service.h"
#include "url/gurl.h"

namespace brave {
BraveBookmarkClient::BraveBookmarkClient(
    sync_bookmarks::BookmarkSyncService* bookmark_sync_service)
    : bookmark_sync_service_(bookmark_sync_service) {}

BraveBookmarkClient::~BraveBookmarkClient() {}

void BraveBookmarkClient::Init(bookmarks::BookmarkModel* model) {
  model_ = model;
}

bool BraveBookmarkClient::PreferTouchIcon() {
  return true;
}

base::CancelableTaskTracker::TaskId
BraveBookmarkClient::GetFaviconImageForPageURL(
    const GURL& page_url,
    favicon_base::IconType type,
    favicon_base::FaviconImageCallback callback,
    base::CancelableTaskTracker* tracker) {
  return base::CancelableTaskTracker::kBadTaskId;
}

bool BraveBookmarkClient::SupportsTypedCountForUrls() {
  return true;
}

void BraveBookmarkClient::GetTypedCountForUrls(
    UrlTypedCountMap* url_typed_count_map) {
}

bool BraveBookmarkClient::IsPermanentNodeVisible(
    const bookmarks::BookmarkPermanentNode* node) {
  return node->type() == bookmarks::BookmarkNode::MOBILE;
}

void BraveBookmarkClient::RecordAction(const base::UserMetricsAction& action) {
  base::RecordAction(action);
}

bookmarks::LoadManagedNodeCallback
BraveBookmarkClient::GetLoadManagedNodeCallback() {
  return bookmarks::LoadManagedNodeCallback();
}

bool BraveBookmarkClient::CanSetPermanentNodeTitle(
    const bookmarks::BookmarkNode* permanent_node) {
  return true;
}

bool BraveBookmarkClient::CanSyncNode(const bookmarks::BookmarkNode* node) {
  return true;
}

bool BraveBookmarkClient::CanBeEditedByUser(
    const bookmarks::BookmarkNode* node) {
  return true;
}

std::string BraveBookmarkClient::EncodeBookmarkSyncMetadata() {
  return bookmark_sync_service_->EncodeBookmarkSyncMetadata();
}

void BraveBookmarkClient::DecodeBookmarkSyncMetadata(
    const std::string& metadata_str,
    const base::RepeatingClosure& schedule_save_closure) {
  bookmark_sync_service_->DecodeBookmarkSyncMetadata(
      metadata_str, schedule_save_closure, model_);
}
}
