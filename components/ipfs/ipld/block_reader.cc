/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/block_reader.h"

#include <memory>
#include <utility>

#include "brave/components/ipfs/ipld/block.h"
#include "brave/components/ipfs/ipld/car_block_reader.h"
#include "brave/components/ipfs/ipld/content_requester.h"

namespace ipfs::ipld {

BlockReader::BlockReader(std::unique_ptr<ContentRequester> content_requester)
    : content_requester_(std::move(content_requester)) {}

BlockReader::~BlockReader() = default;

void BlockReader::Read(BlockReaderCallback callback) {
  if (!callback || !content_requester_) {
    return;
  }

  content_requester_->Request(
      base::BindRepeating(&BlockReader::OnRequestDataReceived,
                          weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

base::raw_ptr<BlockFactory> BlockReader::GetBlockFactory() {
  return block_factory_.get();
}

base::RepeatingCallback<void(std::unique_ptr<std::vector<uint8_t>>, const bool)>
BlockReader::GetReadCallbackForTests(BlockReaderCallback callback) {
  return base::BindRepeating(&BlockReader::OnRequestDataReceived,
                             weak_ptr_factory_.GetWeakPtr(),
                             std::move(callback));
}

BlockReaderFactory::BlockReaderFactory() = default;
BlockReaderFactory::~BlockReaderFactory() = default;

std::unique_ptr<BlockReader> BlockReaderFactory::CreateCarBlockReader(
    const GURL& url,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* prefs) {
  return std::make_unique<CarBlockReader>(
      content_reader_factory_->CreateCarContentRequester(
          url, url_loader_factory, prefs));
}

}  // namespace ipfs::ipld
