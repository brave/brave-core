/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/components/bookmarks/browser/bookmark_model.cc"

#include "ui/base/models/tree_node_iterator.h"

namespace bookmarks {

// Move bookmarks under "Other Bookmarks" folder from
// https://github.com/brave/brave-core/pull/3620 to original permanent node
void BraveMigrateOtherNodeFolder(BookmarkModel* model) {
  CHECK(model);
  CHECK(model->loaded());
  // Model must be loaded at this point
  if (!model->bookmark_bar_node()->children().size())
    return;
  const BookmarkNode* possible_other_node_folder =
      model->bookmark_bar_node()->children().back().get();
  if (possible_other_node_folder->is_folder() &&
      (possible_other_node_folder->GetTitledUrlNodeTitle() ==
       model->other_node()->GetTitledUrlNodeTitle())) {
    size_t children_size = possible_other_node_folder->children().size();
    for (size_t i = 0; i < children_size; ++i) {
      model->Move(possible_other_node_folder->children().front().get(),
                  model->other_node(), i);
    }
    model->Remove(possible_other_node_folder);
  }
}

void BraveClearSyncV1MetaInfo(BookmarkModel* model) {
  CHECK(model);
  CHECK(model->loaded());
  model->BeginExtensiveChanges();
  ui::TreeNodeIterator<const BookmarkNode> iterator(model->root_node());
  while (iterator.has_next()) {
    const BookmarkNode* node = iterator.Next();
    // Permanent node cannot trigger BookmarkModelObserver and we stored meta
    // info in it
    if (model->is_permanent_node(node)) {
      const_cast<BookmarkNode*>(node)->SetMetaInfoMap(
          BookmarkNode::MetaInfoMap());
    }

    model->DeleteNodeMetaInfo(node, "object_id");
    model->DeleteNodeMetaInfo(node, "order");
    model->DeleteNodeMetaInfo(node, "parent_object_id");
    model->DeleteNodeMetaInfo(node, "position_in_parent");
    model->DeleteNodeMetaInfo(node, "sync_timestamp");
    model->DeleteNodeMetaInfo(node, "version");

    // These might exist if user uses v1 since the very beginning when we
    // integrates with chromium sync
    model->DeleteNodeMetaInfo(node, "originator_cache_guid");
    model->DeleteNodeMetaInfo(node, "originator_client_item_id");
    model->DeleteNodeMetaInfo(node, "mtime");
    model->DeleteNodeMetaInfo(node, "ctime");
  }
  model->EndExtensiveChanges();
}

}  // namespace bookmarks
