/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/syncer_helper.h"

#define BRAVE_APPLY_REMOTE_UPDATE true ? brave_sync::GetIndex(new_parent, node):

#define BRAVE_PROCESS_CREATE_1                                              \
  std::string order;                                                        \
  std::string object_id;                                                    \
  for (int i = 0; i < update_entity.specifics.bookmark().meta_info_size();  \
       ++i) {                                                               \
    if (update_entity.specifics.bookmark().meta_info(i).key() == "order") { \
      order = update_entity.specifics.bookmark().meta_info(i).value();      \
    } else if (update_entity.specifics.bookmark().meta_info(i).key() ==     \
               "object_id") {                                               \
      object_id = update_entity.specifics.bookmark().meta_info(i).value();  \
    }                                                                       \
  }

#define BRAVE_PROCESS_CREATE_2 \
  true ? brave_sync::GetIndex(parent_node, order, object_id):
#include "../../../../components/sync_bookmarks/bookmark_remote_updates_handler.cc"
#undef BRAVE_APPLY_REMOTE_UPDATE
#undef BRAVE_PROCESS_CREATE_1
#undef BRAVE_PROCESS_CREATE_2
