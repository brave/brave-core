/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/dag_nodes_collector.h"

#include <sstream>

#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "brave/components/ipfs/ipld/block.h"


namespace ipfs::ipld {

DagNodesCollector::DagNodesCollector() = default;
DagNodesCollector::~DagNodesCollector() = default;

void DagNodesCollector::CollectBlock(std::unique_ptr<Block> block) {
  if (block->IsRoot()) {
    dag_nodes_.erase("");  // remove existing root block, allowed only one root
  }

  dag_nodes_.try_emplace(block->Cid(), std::move(block));
}

Block* DagNodesCollector::GetRootBlock() const {
  return GetBlockByCid("");
}

Block* DagNodesCollector::GetBlockByCid(const std::string& cid) const {
  auto block_iterator = dag_nodes_.find(cid);
  if (block_iterator == dag_nodes_.end()) {
    return nullptr;
  }

  return block_iterator->second.get();
}

void DagNodesCollector::Clear() {
  dag_nodes_.clear();
}

bool DagNodesCollector::IsEmpty() const {
  return dag_nodes_.empty();
}

size_t DagNodesCollector::Size() const {
  return dag_nodes_.size();
}

void DagNodesCollector::Debug() const {

  std::stringstream ss;

  base::ranges::for_each(dag_nodes_, [&](const auto& map_item){
    ss << map_item.first << "\r\n";
  });

  LOG(INFO) << "[INFO] dag_nodes: " << ss.str();
}

}  // namespace ipfs::ipld
