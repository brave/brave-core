/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/bookmarks/exporter/bookmarks_encoder.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "components/bookmarks/browser/bookmark_codec.h"
#include "components/bookmarks/browser/bookmark_node.h"

namespace ios {
namespace bookmarks_encoder {
base::Value::Dict Encode(const bookmarks::BookmarkNode* bookmark_bar_node,
                         const bookmarks::BookmarkNode* other_folder_node,
                         const bookmarks::BookmarkNode* mobile_folder_node) {
  auto encoder = std::make_unique<bookmarks::BookmarkCodec>();
  return std::move(
      encoder
          ->Encode(bookmark_bar_node, other_folder_node, mobile_folder_node,
                   /*model_meta_info_map*/ nullptr,
                   /*model_mmodel_unsynced_meta_info_mapeta_info_map*/ nullptr,
                   /*sync_metadata_str*/ std::string())
          .GetDict());
}
}  // namespace bookmarks_encoder
}  // namespace ios
