/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPLD_CAR_BLOCK_READER_H_
#define BRAVE_COMPONENTS_IPFS_IPLD_CAR_BLOCK_READER_H_

#include <cstdint>
#include "brave/components/ipfs/ipld/block_reader.h"

namespace ipfs::ipld {

class CarBlockReader : public BlockReader {
 public:
  explicit CarBlockReader(std::unique_ptr<ContentRequester> content_requester);
  ~CarBlockReader() override;

 protected:
  void OnRequestDataReceived(BlockReaderCallback callback,
                             std::unique_ptr<std::vector<uint8_t>> data,
                             const bool is_completed) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BlockReaderUnitTest, ReceiveBlocksByChunks);
  friend class BlockReaderUnitTest;
  bool is_header_retrieved_{false};
  std::vector<uint8_t> buffer_;
};

}  // namespace ipfs::ipld

#endif  // BRAVE_COMPONENTS_IPFS_IPLD_CAR_BLOCK_READER_H_
