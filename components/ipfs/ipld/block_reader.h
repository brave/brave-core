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
#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "components/prefs/pref_service.h"
#include "partition_alloc/pointers/raw_ptr.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace ipfs::ipld {

class ContentRequester;
class Block;
class BlockFactory;

class BlockReader {
 public:
  using BlockReaderCallback =
      base::RepeatingCallback<void(std::unique_ptr<Block>, bool)>;

  virtual ~BlockReader();

  virtual void Read(BlockReaderCallback callback);

  base::raw_ptr<BlockFactory> GetBlockFactory();

 protected:
  explicit BlockReader(std::unique_ptr<ContentRequester> content_requester);

  virtual void OnRequestDataReceived(BlockReaderCallback callback,
                                     std::unique_ptr<std::vector<uint8_t>> data,
                                     const bool is_completed) = 0;

 private:
  FRIEND_TEST_ALL_PREFIXES(BlockReaderUnitTest, BasicTestSteps);
  FRIEND_TEST_ALL_PREFIXES(BlockReaderUnitTest, ReceiveBlocksByChunks);
  friend class BlockReaderUnitTest;

  base::RepeatingCallback<void(std::unique_ptr<std::vector<uint8_t>>,
                               const bool)>
  GetReadCallbackForTests(BlockReaderCallback callback);

  std::unique_ptr<BlockFactory> block_factory_{
      std::make_unique<BlockFactory>()};
  std::unique_ptr<ContentRequester> content_requester_;
  base::WeakPtrFactory<BlockReader> weak_ptr_factory_{this};
};

class ContentRequesterFactory;
class BlockReaderFactory {
 public:
  BlockReaderFactory();
  ~BlockReaderFactory();
  BlockReaderFactory(const BlockReaderFactory&) = delete;
  BlockReaderFactory(BlockReaderFactory&&) = delete;
  BlockReaderFactory& operator=(const BlockReaderFactory&) = delete;
  BlockReaderFactory& operator=(BlockReaderFactory&&) = delete;

  std::unique_ptr<BlockReader> CreateCarBlockReader(
      const GURL& url,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* prefs);

 private:
  std::unique_ptr<ContentRequesterFactory> content_reader_factory_{
      std::make_unique<ContentRequesterFactory>()};
};

}  // namespace ipfs::ipld

#endif  // BRAVE_COMPONENTS_IPFS_IPLD_BLOCK_READER_H_
