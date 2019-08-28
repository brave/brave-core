/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sync_bookmarks/bookmark_change_processor.h"

#include "brave/components/brave_sync/syncer_helper.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/sync/syncable/base_transaction.h"
#include "components/sync/syncable/syncable_base_transaction.h"
#include "components/sync/syncable/syncable_write_transaction.h"
#include "components/sync/syncable/write_node.h"
#include "components/sync/syncable/write_transaction.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkModelObserver;
using bookmarks::BookmarkNode;
using sync_bookmarks::BookmarkChangeProcessor;

namespace {

class ScopedPauseObserver {
 public:
  explicit ScopedPauseObserver(BookmarkModel* model,
                               BookmarkModelObserver* observer)
      : model_(model), observer_(observer) {
    DCHECK_NE(observer_, nullptr);
    DCHECK_NE(model_, nullptr);
    model_->RemoveObserver(observer_);
  }
  ~ScopedPauseObserver() { model_->AddObserver(observer_); }

 private:
  BookmarkModel* model_;             // Not owned
  BookmarkModelObserver* observer_;  // Not owned
};

bool IsFirstLoadedFavicon(BookmarkChangeProcessor* bookmark_change_processor,
                          BookmarkModel* bookmark_model,
                          const BookmarkNode* node) {
  // Avoid sending duplicate records right after applying CREATE records,
  // BookmarkChangeProcessor::SetBookmarkFavicon, put favicon data into database
  // BookmarkNode::favicon() and BookmarkNode::icon_url() are available only
  // after first successfuly BookmarkModel::GetFavicon() which means
  // BookmarkModel::OnFaviconDataAvailable has image result available.
  // So we set metainfo to know if it is first time favicon load after create
  // node from remote record
  std::string FirstLoadedFavicon;
  if (node->GetMetaInfo("FirstLoadedFavicon", &FirstLoadedFavicon)) {
    if (!node->icon_url())
      return true;
    ScopedPauseObserver pause(bookmark_model, bookmark_change_processor);
    BookmarkNode* mutable_node = const_cast<BookmarkNode*>(node);
    mutable_node->DeleteMetaInfo("FirstLoadedFavicon");
    return true;
  }
  return false;
}

}  // namespace

namespace sync_bookmarks {

void BookmarkChangeProcessor::MoveSyncNode(
    int index,
    const bookmarks::BookmarkNode* node,
    const syncer::BaseTransaction* trans) {
  syncer::WriteTransaction write_trans(
      FROM_HERE, trans->GetUserShare(),
      static_cast<syncer::syncable::WriteTransaction*>(
          trans->GetWrappedTrans()));
  syncer::WriteNode sync_node(&write_trans);
  if (!model_associator_->InitSyncNodeFromChromeId(node->id(), &sync_node)) {
    syncer::SyncError error(FROM_HERE, syncer::SyncError::DATATYPE_ERROR,
                            "Failed to init sync node from chrome node",
                            syncer::BOOKMARKS);
    error_handler()->OnUnrecoverableError(error);
    return;
  }

  if (!PlaceSyncNode(MOVE, node->parent(), index, &write_trans, &sync_node,
                     model_associator_)) {
    syncer::SyncError error(FROM_HERE, syncer::SyncError::DATATYPE_ERROR,
                            "Failed to place sync node", syncer::BOOKMARKS);
    error_handler()->OnUnrecoverableError(error);
    return;
  }
}

}  // namespace sync_bookmarks

#define BRAVE_BOOKMARK_CHANGE_PROCESSOR_BOOKMARK_NODE_FAVICON_CHANGED \
  if (IsFirstLoadedFavicon(this, bookmark_model_, node))              \
    return;

#define BRAVE_BOOKMARK_CHANGE_PROCESSOR_UPDATE_SYNC_NODE_PROPERTIES \
  brave_sync::AddBraveMetaInfo(src, model);

#define BRAVE_BOOKMARK_CHANGE_PROCESSOR_BOOKMARK_NODE_MOVED \
  ScopedPauseObserver pause(bookmark_model_, this);         \
  brave_sync::AddBraveMetaInfo(child, model);               \
  SetSyncNodeMetaInfo(child, &sync_node);

#define BRAVE_BOOKMARK_CHANGE_PROCESSOR_CHILDREN_REORDERED \
  ScopedPauseObserver pause(bookmark_model_, this);        \
  brave_sync::AddBraveMetaInfo(child, model);              \
  SetSyncNodeMetaInfo(child, &sync_child);

#define BRAVE_BOOKMARK_CHANGE_PROCESSOR_APPLY_CHANGES_FROM_SYNC_MODEL_1 \
  bookmark_model_->SetNodeMetaInfo(dst, "FirstLoadedFavicon", "true");

#define BRAVE_BOOKMARK_CHANGE_PROCESSOR_APPLY_CHANGES_FROM_SYNC_MODEL_2   \
  int new_index =                                                         \
      brave_sync::GetIndexByCompareOrderStartFrom(parent, it->second, 0); \
  if (it->first != new_index) {                                           \
    model->Move(it->second, parent, new_index);                           \
    MoveSyncNode(new_index, it->second, trans);                           \
  } else  // NOLINT

#include "../../../../components/sync_bookmarks/bookmark_change_processor.cc"  // NOLINT
#undef BRAVE_BOOKMARK_CHANGE_PROCESSOR_BOOKMARK_NODE_FAVICON_CHANGED
#undef BRAVE_BOOKMARK_CHANGE_PROCESSOR_UPDATE_SYNC_NODE_PROPERTIES
#undef BRAVE_BOOKMARK_CHANGE_PROCESSOR_BOOKMARK_NODE_MOVED
#undef BRAVE_BOOKMARK_CHANGE_PROCESSOR_CHILDREN_REORDERED
#undef BRAVE_BOOKMARK_CHANGE_PROCESSOR_APPLY_CHANGES_FROM_SYNC_MODEL_1
#undef BRAVE_BOOKMARK_CHANGE_PROCESSOR_APPLY_CHANGES_FROM_SYNC_MODEL_2
