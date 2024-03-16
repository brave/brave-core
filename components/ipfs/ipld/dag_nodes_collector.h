/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPLD_DAG_NODES_COLLECTOR_H_
#define BRAVE_COMPONENTS_IPFS_IPLD_DAG_NODES_COLLECTOR_H_

#include <unordered_map>
#include "brave/components/ipfs/ipld/trustless_client_types.h"

namespace ipfs::ipld {

class Block;

class DagNodesCollector {
 public:
  using BlockCollectorMap = std::unordered_map<std::string,
                                               std::unique_ptr<Block>,
                                               StringHash,
                                               std::equal_to<>>;

  DagNodesCollector();
  ~DagNodesCollector();

  void CollectBlock(std::unique_ptr<Block> block);
  Block* GetRootBlock() const;
  Block* GetBlockByCid(const std::string& cid) const;
  bool IsEmpty() const;
  void Clear();
  void Debug() const;
 private:
  BlockCollectorMap dag_nodes_;
};

}  // namespace ipfs::ipld

#endif  // BRAVE_COMPONENTS_IPFS_IPLD_DAG_NODES_COLLECTOR_H_
