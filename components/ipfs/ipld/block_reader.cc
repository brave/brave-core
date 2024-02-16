/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/block_reader.h"

#include <memory>
#include <utility>

#include "brave/components/ipfs/ipld/content_requester.h"
#include "brave/components/ipfs/ipld/block.h"


namespace ipfs::ipld {

BlockReader::BlockReader(std::unique_ptr<ContentRequester> content_requester)
    : block_factory_(std::make_unique<BlockFactory>()), content_requester_(std::move(content_requester)) {}

BlockReader::~BlockReader(){}

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

/*
void BlockReader::OnRequestDataReceived(
    BlockReaderCallback callback,
    std::unique_ptr<std::vector<uint8_t>> data,
    const bool is_success) {

  base::span<const uint8_t> buffer_span(*data);// TODO change it to member variable to have it like a buffer, in case we will have several data parts received

  while (!buffer_span.empty()){
    uint32_t received_buffer_size = buffer_span.size(); 
    if(received_buffer_size < sizeof(uint64_t)) {
      LOG(INFO) << "[IPFS] Need more bytes for block length field !!!";
      return;
    }

    uint64_t current_block_size = DecodeBlockLength(buffer_span);
    LOG(INFO) << "[IPFS] read data_block_size_:" << current_block_size;  
    
    if(received_buffer_size < current_block_size) {
      LOG(INFO) << "[IPFS] Need more block bytes !!! current_buffer_size:" << received_buffer_size ;
      return;
    }

    LOG(INFO) << "[IPFS] buffer_span.size:" << buffer_span.size() <<  " block_size:" << current_block_size << " block_size:" << current_block_size;
    LOG(INFO) << "[IPFS] buffer_span[0]:" << (int)buffer_span[0] << " buffer_span[1]:" << (int)buffer_span[1];
    base::span block_span = buffer_span;
    if(current_block_size < buffer_span.size()) {
      block_span = buffer_span.subspan(0, current_block_size);
      LOG(INFO) << "[IPFS] block_span[0]:" << (int)block_span[0] << " block_span[1]:" << (int)block_span[1];    
      LOG(INFO) << "[IPFS] block_span[end-1]:" << (int)block_span[block_span.size()-1-1] << " block_span[end]:" << (int)block_span[block_span.size() - 1];
      buffer_span = buffer_span.subspan(current_block_size);
    } else {
      buffer_span = base::span<uint8_t>();
    }

   // buffer_span = buffer_span.subspan(block_size, buffer_span.size());
    //const auto block_end_offset = block_size - (data->size() - buffer_span.size());
    //auto block_data_span = data_span.subspan(0, block_end_offset);
    const std::vector<uint8_t> vec(block_span.begin(), block_span.end());

    LOG(INFO) << "[IPFS] vec.size:" << vec.size() << " vec[0]:" << (int)vec[0] << " vec[1]:" << (int)vec[1] ;

    if (!is_header_retrieved_) {
      auto carv1_header_result = DecodeCarv1Header(vec);    
      if (carv1_header_result.error.error_code != 0) {
        LOG(INFO) << "[IPFS] Could not decode!!! error:" << carv1_header_result.error.error.c_str();
        return;
      }
      LOG(INFO) << "[IPFS] Roots[0]:" << carv1_header_result.data.roots[0].c_str();
      is_header_retrieved_ = true;
      current_block_size = 0; // reset block length counter
    } else {

      auto block_info_result = DecodeBlockInfo(0, vec);
      if (block_info_result.error.error_code != 0) {
        LOG(INFO) << "[IPFS] Could not decode block!!! error:" << block_info_result.error.error.c_str();
        return;
      }
      LOG(INFO) << "[IPFS] block_info:" << block_info_result.data.c_str();
    }
  }
}
*/

}  // namespace ipfs::ipld
