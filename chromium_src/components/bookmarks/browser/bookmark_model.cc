/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../components/bookmarks/browser/bookmark_model.cc"

namespace bookmarks {

// Move bookmarks under "Other Bookmarks" permanent node to a same name folder
// at the end of "Bookmark Bar" permanent node
void BraveMigrateOtherNode(BookmarkModel* model) {
  DCHECK(model);
  if (!model->other_node()->children().empty()) {
    const bookmarks::BookmarkNode* new_other_node =
        model->AddFolder(model->bookmark_bar_node(),
                         model->bookmark_bar_node()->children().size(),
                         model->other_node()->GetTitledUrlNodeTitle());
    size_t children_size = model->other_node()->children().size();
    for (size_t i = 0; i < children_size; ++i) {
      model->Move(model->other_node()->children().front().get(), new_other_node,
                  i);
    }
  }
}

}  // namespace bookmarks
