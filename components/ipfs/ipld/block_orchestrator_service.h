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
#include "components/keyed_service/core/keyed_service.h"

namespace ipfs::ipld {

class BlockOrchestratorService : public KeyedService {
 public:
  explicit BlockOrchestratorService(PrefService* pref_service);
  ~BlockOrchestratorService() override;

  BlockOrchestratorService(const BlockOrchestratorService&) = delete;
  BlockOrchestratorService(BlockOrchestratorService&&) = delete;
  BlockOrchestratorService& operator=(const BlockOrchestratorService&) = delete;
  BlockOrchestratorService& operator=(BlockOrchestratorService&&) = delete;

  void BuildResponse(std::unique_ptr<IpfsTrustlessRequest> request,
                     IpfsRequestCallback callback);

  bool IsActive() const;
 private:
  FRIEND_TEST_ALL_PREFIXES(BlockOrchestratorUnitTest, RequestFile);
  friend class BlockOrchestratorUnitTest;

  void OnBlockRead(std::unique_ptr<Block> block, bool is_completed);
  void Reset();

  void ProcessTarget(std::unique_ptr<TrustlessTarget> target) ;

  std::unordered_map<std::string,
                     std::unique_ptr<Block>,
                     StringHash,
                     std::equal_to<>> dag_nodes_;

  void BlockChainForCid(Block* block) const;

  // void CreateDagPathIndex();
  // std::unordered_map<std::string,
  //                    Block*,
  //                    StringHash,
  //                    std::equal_to<>> darg_nodes_path_index_;

  IpfsRequestCallback request_callback_;
  std::unique_ptr<IpfsTrustlessRequest> request_;
  std::unique_ptr<BlockReader> block_reader_;
  raw_ptr<PrefService> pref_service_;
  base::WeakPtrFactory<BlockOrchestratorService> weak_ptr_factory_{this};
};

}  // namespace ipfs::ipld

#endif  // BRAVE_COMPONENTS_IPFS_IPLD_BLOCK_ORCHESTRATOR_H_
