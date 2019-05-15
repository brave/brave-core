/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/syncer_helper.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_sync/bookmark_order_util.h"
#include "brave/components/brave_sync/tools.h"
#include "components/bookmarks/browser/bookmark_model.h"

namespace brave_sync {
namespace {

void ClearOrder(const bookmarks::BookmarkNode* leaf) {
  bookmarks::BookmarkNode* node = const_cast<bookmarks::BookmarkNode*>(leaf);
  while (!node->is_permanent_node()) {
    node->DeleteMetaInfo("order");
    node = node->parent();
  }
}

void SetOrder(bookmarks::BookmarkModel* model,
              const bookmarks::BookmarkNode* node,
              const std::string& parent_order) {
  DCHECK(!parent_order.empty());
  int index = node->parent()->GetIndexOf(node);
  std::string order = parent_order + "." + base::NumberToString(index + 1);
  model->SetNodeMetaInfo(node, "order", order);
}

void ReCalculateOrder(bookmarks::BookmarkModel* model,
                      const bookmarks::BookmarkNode* node) {
  DCHECK(!node->is_permanent_node());
  std::string parent_order;
  const bookmarks::BookmarkNode* parent = node->parent();
  parent->GetMetaInfo("order", &parent_order);
  if (parent_order.empty()) {
    ReCalculateOrder(model, node->parent());
  } else {
    SetOrder(model, node, parent_order);
  }
}

uint64_t GetIndexByOrder(const std::string& record_order) {
  uint64_t index = 0;
  size_t last_dot_index = record_order.rfind(".");
  DCHECK(last_dot_index != std::string::npos);
  std::string last_digit = record_order.substr(last_dot_index + 1);
  bool result = base::StringToUint64(last_digit, &index);
  --index;
  DCHECK(index >= 0);
  DCHECK(result);
  return index;
}

}   // namespace

void AddBraveMetaInfo(
    const bookmarks::BookmarkNode* node,
    bookmarks::BookmarkModel* model,
    bool has_new_parent) {
  if (has_new_parent) {
    ClearOrder(node);
    ReCalculateOrder(model, node);
  } else {
    std::string parent_order;
    node->parent()->GetMetaInfo("order", &parent_order);
    SetOrder(model, node, parent_order);
  }

  std::string object_id;
  node->GetMetaInfo("object_id", &object_id);
  // newly created node
  if (object_id.empty()) {
    object_id = tools::GenerateObjectId();
  }
  model->SetNodeMetaInfo(node, "object_id", object_id);

  std::string parent_object_id;
  node->parent()->GetMetaInfo("object_id", &parent_object_id);
  model->SetNodeMetaInfo(node, "parent_object_id", parent_object_id);

  std::string sync_timestamp;
  node->GetMetaInfo("sync_timestamp", &sync_timestamp);
  if (sync_timestamp.empty()) {
    sync_timestamp = std::to_string(base::Time::Now().ToJsTime());
  }
  DCHECK(!sync_timestamp.empty());
  model->SetNodeMetaInfo(node, "sync_timestamp", sync_timestamp);
}

uint64_t GetIndex(const bookmarks::BookmarkNode* parent,
                  const bookmarks::BookmarkNode* src) {
  int index = 0;
  std::string src_order;
  src->GetMetaInfo("order", &src_order);
  DCHECK(!src_order.empty());
  index = GetIndexByOrder(src_order);
  const bookmarks::BookmarkNode* node = parent->GetChild(index);
  if (node) {
    std::string node_order;
    node->GetMetaInfo("order", &node_order);

    if (!node_order.empty() &&
        CompareOrder(node_order, src_order))
      return index + 1;
  }
  return index;
}

}   // namespace brave_sync
