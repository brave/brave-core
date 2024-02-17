/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPLD_BLOCK_READER_H_
#define BRAVE_COMPONENTS_IPFS_IPLD_BLOCK_READER_H_

#include <cstdint>
#include <memory>
#include <vector>
#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "partition_alloc/pointers/raw_ptr.h"

namespace ipfs::ipld {

class ContentRequester;
class Block;
class BlockFactory;

class BlockReader {
 public:
  using BlockReaderCallback =
      base::RepeatingCallback<void(std::unique_ptr<Block>)>;

  virtual ~BlockReader(); 

  virtual void Read(BlockReaderCallback callback);

  base::raw_ptr<BlockFactory> GetBlockFactory();

 protected:
  explicit BlockReader(std::unique_ptr<ContentRequester> content_requester);

  virtual void OnRequestDataReceived(BlockReaderCallback callback,
                                     std::unique_ptr<std::vector<uint8_t>> data,
                                     const bool is_success) = 0;

 private:
  std::unique_ptr<BlockFactory> block_factory_{std::make_unique<BlockFactory>()};
  std::unique_ptr<ContentRequester> content_requester_;
  base::WeakPtrFactory<BlockReader> weak_ptr_factory_{this};
};

class BlockReaderFactory {
 public:
  BlockReaderFactory() = default;
  ~BlockReaderFactory() = default;
  BlockReaderFactory(const BlockReaderFactory&) = delete;
  BlockReaderFactory(BlockReaderFactory&&) = delete;
  BlockReaderFactory& operator=(const BlockReaderFactory&) = delete;
  BlockReaderFactory& operator=(BlockReaderFactory&&) = delete;

  std::unique_ptr<BlockReader> CreateCarBlockReader();
};

}  // namespace ipfs::ipld

#endif  // BRAVE_COMPONENTS_IPFS_IPLD_BLOCK_READER_H_
