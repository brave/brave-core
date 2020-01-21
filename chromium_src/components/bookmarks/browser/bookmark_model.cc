/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../components/bookmarks/browser/bookmark_model.cc"

namespace bookmarks {

// Move bookmarks under "Other Bookmarks" folder from
// https://github.com/brave/brave-core/pull/3620 to original permanent node
void BraveMigrateOtherNodeFolder(BookmarkModel* model) {
  CHECK(model);
  CHECK(model->loaded());
  // Model must be loaded at this point
  if (!model->bookmark_bar_node()->children().size())
    return;
  const bookmarks::BookmarkNode* possible_other_node_folder =
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

}  // namespace bookmarks
