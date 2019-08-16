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

void SetOrder(bookmarks::BookmarkModel* model,
              const bookmarks::BookmarkNode* node,
              const std::string& parent_order) {
  DCHECK(!parent_order.empty());
  int index = node->parent()->GetIndexOf(node);

  bookmarks::BookmarkNode* parent =
      const_cast<bookmarks::BookmarkNode*>(node->parent());

  auto* prev_node = index == 0 ? nullptr : parent->GetChild(index - 1);
  auto* next_node = index == parent->child_count() - 1
                        ? nullptr
                        : parent->GetChild(index + 1);

  std::string prev_order;
  std::string next_order;
  if (prev_node)
    prev_node->GetMetaInfo("order", &prev_order);

  if (next_node)
    next_node->GetMetaInfo("order", &next_order);

  std::string order =
      brave_sync::GetOrder(prev_order, next_order, parent_order);
  model->SetNodeMetaInfo(node, "order", order);
}

uint64_t GetIndexByOrder(const std::string& record_order) {
  uint64_t index = 0;
  size_t last_dot_index = record_order.rfind(".");
  DCHECK(last_dot_index != std::string::npos);
  std::string last_digit = record_order.substr(last_dot_index + 1);
  bool result = base::StringToUint64(last_digit, &index);
  --index;
  DCHECK_GE(index, 0u);
  DCHECK(result);
  return index;
}

}  // namespace

uint64_t GetIndexByCompareOrderStartFrom(const bookmarks::BookmarkNode* parent,
                                         const bookmarks::BookmarkNode* src,
                                         int index) {
  std::string src_order;
  src->GetMetaInfo("order", &src_order);
  DCHECK(!src_order.empty());
  DCHECK_GE(index, 0);
  bool use_order = true;  // If false use object_id
  std::string src_object_id;
  while (index < parent->child_count()) {
    const bookmarks::BookmarkNode* node = parent->GetChild(index);
    if (src->id() == node->id()) {
      // We reached ourselves, no sense to go further, because we know all
      // unsorted elements are in the end
      return index;
    }

    if (use_order) {
      std::string node_order;
      node->GetMetaInfo("order", &node_order);
      if (!node_order.empty() &&
          brave_sync::CompareOrder(src_order, node_order)) {
        return index;
      }

      if (src_order == node_order) {
        use_order = false;
      }
    }

    if (!use_order) {
      if (src_object_id.empty()) {
        src->GetMetaInfo("object_id", &src_object_id);
      }

      std::string node_object_id;
      node->GetMetaInfo("object_id", &node_object_id);

      if (src_object_id < node_object_id) {
        return index;
      }
    }
    ++index;
  }
  return index;
}

void AddBraveMetaInfo(const bookmarks::BookmarkNode* node,
                      bookmarks::BookmarkModel* model) {
  std::string parent_order;
  node->parent()->GetMetaInfo("order", &parent_order);
  SetOrder(model, node, parent_order);

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
    model->SetNodeMetaInfo(node, "sync_timestamp", sync_timestamp);
  }
  DCHECK(!sync_timestamp.empty());
}

uint64_t GetIndex(const bookmarks::BookmarkNode* parent,
                  const bookmarks::BookmarkNode* src) {
  DCHECK(parent);
  DCHECK(src);
  int index = 0;
  std::string src_order;
  src->GetMetaInfo("order", &src_order);
  DCHECK(!src_order.empty());
  index = GetIndexByOrder(src_order);
  if (index < parent->child_count()) {
    const bookmarks::BookmarkNode* node = parent->GetChild(index);
    if (node) {
      std::string node_order;
      node->GetMetaInfo("order", &node_order);

      DCHECK(!node_order.empty());
      if (CompareOrder(node_order, src_order))
        return index + 1;
    }
  }
  return index;
}

}  // namespace brave_sync
