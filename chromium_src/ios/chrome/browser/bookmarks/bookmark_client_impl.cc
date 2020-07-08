/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/bookmarks/bookmark_client_impl.h"

#include "base/logging.h"
#include "base/metrics/user_metrics.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/bookmarks/browser/base_bookmark_model_observer.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/browser/bookmark_storage.h"
#include "components/bookmarks/managed/managed_bookmark_service.h"
#include "components/favicon_base/favicon_types.h"
#include "components/sync_bookmarks/bookmark_sync_service.h"
#include "url/gurl.h"

BookmarkClientImpl::BookmarkClientImpl(
    ChromeBrowserState* browser_state,
    bookmarks::ManagedBookmarkService* managed_bookmark_service,
    sync_bookmarks::BookmarkSyncService* bookmark_sync_service)
    : browser_state_(browser_state),
      managed_bookmark_service_(managed_bookmark_service),
      bookmark_sync_service_(bookmark_sync_service) {
  // workaround for unsued variables browser_state_ and  managed_bookmark_service_
  DCHECK(browser_state_ || managed_bookmark_service_ || bookmark_sync_service_);
}

BookmarkClientImpl::~BookmarkClientImpl() {}

void BookmarkClientImpl::Init(bookmarks::BookmarkModel* model) {
  model_ = model;
}

bool BookmarkClientImpl::PreferTouchIcon() {
  return true;
}

base::CancelableTaskTracker::TaskId
BookmarkClientImpl::GetFaviconImageForPageURL(
    const GURL& page_url,
    favicon_base::IconType type,
    favicon_base::FaviconImageCallback callback,
    base::CancelableTaskTracker* tracker) {
  return base::CancelableTaskTracker::kBadTaskId;
}

bool BookmarkClientImpl::SupportsTypedCountForUrls() {
  return true;
}

void BookmarkClientImpl::GetTypedCountForUrls(
    UrlTypedCountMap* url_typed_count_map) {
}

bool BookmarkClientImpl::IsPermanentNodeVisibleWhenEmpty(bookmarks::BookmarkNode::Type type) {
  return type == bookmarks::BookmarkNode::MOBILE;
}

void BookmarkClientImpl::RecordAction(const base::UserMetricsAction& action) {
  base::RecordAction(action);
}

bookmarks::LoadManagedNodeCallback
BookmarkClientImpl::GetLoadManagedNodeCallback() {
  return bookmarks::LoadManagedNodeCallback();
}

bool BookmarkClientImpl::CanSetPermanentNodeTitle(
    const bookmarks::BookmarkNode* permanent_node) {
  return true;
}

bool BookmarkClientImpl::CanSyncNode(const bookmarks::BookmarkNode* node) {
  return true;
}

bool BookmarkClientImpl::CanBeEditedByUser(
    const bookmarks::BookmarkNode* node) {
  return true;
}

std::string BookmarkClientImpl::EncodeBookmarkSyncMetadata() {
  return bookmark_sync_service_->EncodeBookmarkSyncMetadata();
}

void BookmarkClientImpl::DecodeBookmarkSyncMetadata(
    const std::string& metadata_str,
    const base::RepeatingClosure& schedule_save_closure) {
  bookmark_sync_service_->DecodeBookmarkSyncMetadata(
      metadata_str, schedule_save_closure, model_);
}
