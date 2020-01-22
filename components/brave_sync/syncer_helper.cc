/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/syncer_helper.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_sync/bookmark_order_util.h"
#include "brave/components/brave_sync/tools.h"
#include "components/bookmarks/browser/bookmark_node.h"

namespace brave_sync {
namespace {

// Get mutable node to prevent BookmarkMetaInfoChanged from being triggered
bookmarks::BookmarkNode* AsMutable(const bookmarks::BookmarkNode* node) {
  return const_cast<bookmarks::BookmarkNode*>(node);
}

void SetOrder(const bookmarks::BookmarkNode* node,
              const std::string& parent_order) {
  DCHECK(!parent_order.empty());
  int index = node->parent()->GetIndexOf(node);

  bookmarks::BookmarkNode* parent =
      const_cast<bookmarks::BookmarkNode*>(node->parent());

  auto* prev_node = index == 0 ? nullptr : parent->children()[index - 1].get();
  auto* next_node = static_cast<size_t>(index) == parent->children().size() - 1
                        ? nullptr
                        : parent->children()[index + 1].get();

  std::string prev_order;
  std::string next_order;
  if (prev_node)
    prev_node->GetMetaInfo("order", &prev_order);

  if (next_node)
    next_node->GetMetaInfo("order", &next_order);

  std::string order =
      brave_sync::GetOrder(prev_order, next_order, parent_order);
  AsMutable(node)->SetMetaInfo("order", order);
}

}  // namespace

size_t GetIndex(const bookmarks::BookmarkNode* parent,
                const std::string& order,
                const std::string& object_id) {
  DCHECK(!order.empty());
  DCHECK(!object_id.empty());
  for (size_t i = 0; i < parent->children().size(); ++i) {
    const bookmarks::BookmarkNode* child = parent->children()[i].get();
    std::string child_order;
    child->GetMetaInfo("order", &child_order);
    if (!child_order.empty() &&
        brave_sync::CompareOrder(order, child_order)) {
      return i;
    } else if (order == child_order) {
      std::string child_object_id;
      child->GetMetaInfo("object_id", &child_object_id);
      if (object_id <= child_object_id) {
        return i;
      }
    }
  }
  return parent->children().size();
}

size_t GetIndex(const bookmarks::BookmarkNode* parent,
                const bookmarks::BookmarkNode* node) {
  std::string order;
  node->GetMetaInfo("order", &order);
  std::string object_id;
  node->GetMetaInfo("object_id", &object_id);

  return GetIndex(parent, order, object_id);
}

void AddBraveMetaInfo(const bookmarks::BookmarkNode* node) {
  std::string parent_order;
  node->parent()->GetMetaInfo("order", &parent_order);
  SetOrder(node, parent_order);

  std::string object_id;
  node->GetMetaInfo("object_id", &object_id);
  // newly created node
  if (object_id.empty()) {
    object_id = tools::GenerateObjectId();
  }
  AsMutable(node)->SetMetaInfo("object_id", object_id);

  std::string parent_object_id;
  node->parent()->GetMetaInfo("object_id", &parent_object_id);
  AsMutable(node)->SetMetaInfo("parent_object_id", parent_object_id);

  std::string sync_timestamp;
  node->GetMetaInfo("sync_timestamp", &sync_timestamp);
  if (sync_timestamp.empty()) {
    sync_timestamp = std::to_string(base::Time::Now().ToJsTime());
    AsMutable(node)->SetMetaInfo("sync_timestamp", sync_timestamp);
  }
  DCHECK(!sync_timestamp.empty());
}

}  // namespace brave_sync
