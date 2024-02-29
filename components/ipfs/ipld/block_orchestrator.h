/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPLD_BLOCK_ORCHESTRATOR_H_
#define BRAVE_COMPONENTS_IPFS_IPLD_BLOCK_ORCHESTRATOR_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/functional/callback.h"
#include "brave/components/ipfs/ipld/block.h"
#include "brave/components/ipfs/ipld/block_reader.h"
#include "brave/components/ipfs/ipld/trustless_client_types.h"
#include "partition_alloc/pointers/raw_ptr.h"

namespace ipfs::ipld {
struct StringHash {
  using is_transparent = void;
  std::size_t operator()(std::string_view sv) const {
    std::hash<std::string_view> hasher;
    return hasher(sv);
  }
};

class BlockOrchestrator {
 public:

  explicit BlockOrchestrator(PrefService* pref_service);
  ~BlockOrchestrator();

  BlockOrchestrator(const BlockOrchestrator&) = delete;
  BlockOrchestrator(BlockOrchestrator&&) = delete;
  BlockOrchestrator& operator=(const BlockOrchestrator&) = delete;
  BlockOrchestrator& operator=(BlockOrchestrator&&) = delete;

  void BuildResponse(std::unique_ptr<IpfsRequest> request,
                     IpfsRequestCallback callback);

 private:
  void OnBlockRead(std::unique_ptr<Block> block, bool is_completed);

  std::unordered_map<std::string, std::unique_ptr<Block>, StringHash, std::equal_to<>>
      dag_nodes_;
  IpfsRequestCallback request_callback_;
  std::unique_ptr<BlockReaderFactory> block_reader_factory_{
      std::make_unique<BlockReaderFactory>()};
  raw_ptr<PrefService> pref_service_;
  base::WeakPtrFactory<BlockOrchestrator> weak_ptr_factory_{this};
};

}  // namespace ipfs::ipld

#endif  // BRAVE_COMPONENTS_IPFS_IPLD_BLOCK_ORCHESTRATOR_H_
