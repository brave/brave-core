/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/car_block_reader.h"
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>
#include "absl/types/optional.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/ranges/algorithm.h"
#include "base/values.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/ipld/block.h"
#include "brave/components/ipfs/ipld/content_requester.h"
#include "brave/components/ipfs/ipld/ipld_utils.h"

namespace {

uint64_t DecodeBlockLength(base::span<const uint8_t>& buffer) {
  int64_t length = 0;
  buffer = ipfs::DecodeVarInt(buffer, &length);
  return static_cast<uint64_t>(length);
}

absl::optional<base::Value> ParseJsonHelper(
    base::StringPiece json,
    absl::optional<base::Value::Type> expected_type) {
  auto result = base::JSONReader::ReadAndReturnValueWithError(
      json,
      base::JSON_PARSE_CHROMIUM_EXTENSIONS | base::JSON_ALLOW_TRAILING_COMMAS);
  if (!result.has_value()) {
    LOG(INFO) << "[IPFS] json:" << json;
    return absl::nullopt;
  }

  if (expected_type && result->type() != *expected_type) {
    return absl::nullopt;
  }
  return std::move(*result);
}

}  // namespace

namespace ipfs::ipld {

CarBlockReader::CarBlockReader(
    std::unique_ptr<ContentRequester> content_requester)
    : BlockReader(std::move(content_requester)) {}
CarBlockReader::~CarBlockReader() = default;

void CarBlockReader::OnRequestDataReceived(
    BlockReaderCallback callback,
    std::unique_ptr<std::vector<uint8_t>> data,
    const bool is_success) {
  base::span<const uint8_t> buffer_span(
      *data);  // TODO change it to member variable to have it like a buffer, in
               // case we will have several data parts received

  while (!buffer_span.empty()) {
    uint64_t received_buffer_size = buffer_span.size();
    if (received_buffer_size < sizeof(uint64_t)) {
      //   LOG(INFO) << "[IPFS] Need more bytes for block length field !!!";
      return;
    }

    uint64_t current_block_size = DecodeBlockLength(buffer_span);
    if (received_buffer_size < current_block_size) {
      //   LOG(INFO) << "[IPFS] Need more block bytes !!! current_buffer_size:"
      //             << received_buffer_size;
      return;
    }

    base::span block_span = buffer_span;
    if (current_block_size <= buffer_span.size()) {
      block_span = buffer_span.subspan(0, current_block_size);
      buffer_span = buffer_span.subspan(current_block_size);
    } else {
      buffer_span = base::span<uint8_t>();
    }

    const std::vector<uint8_t> vec(block_span.begin(), block_span.end());

    // LOG(INFO) << "[IPFS] vec.size:" << vec.size() << " vec[0]:" <<
    // (int)vec[0]
    //           << " vec[1]:" << (int)vec[1];

    if (!is_header_retrieved_) {
      auto carv1_header_result = DecodeCarv1Header(vec);
      if (carv1_header_result.error.error_code != 0) {
        // LOG(INFO) << "[IPFS] Could not decode!!! error:"
        //           << carv1_header_result.error.error.c_str();
        return;
      }
      LOG(INFO) << "[IPFS] Roots[0]:"
                << carv1_header_result.data.roots[0].c_str();
      is_header_retrieved_ = true;
//      current_block_size = 0;  // reset block length counter

      base::Value::List roots_items;
      base::ranges::for_each(
          carv1_header_result.data.roots,
          [&roots_items](auto& item) { roots_items.Append(item.c_str()); });
      base::Value::Dict roots_dict;
      roots_dict.Set("roots", std::move(roots_items));

      callback.Run(GetBlockFactory()->CreateCarBlock(
          "", base::Value(std::move(roots_dict)), nullptr, absl::nullopt));
      
      continue;
    } 
    
    auto block_info_result = DecodeBlockInfo(0, vec);
    if (block_info_result.error.error_code != 0) {
      LOG(INFO) << "[IPFS] Could not decode block!!! error:"
                << block_info_result.error.error.c_str();
      return;
    }

    if (!block_info_result.is_content) {
      auto json_value = ParseJsonHelper(block_info_result.data.c_str(),
                                        base::Value::Type::DICT);
      // LOG(INFO) << "[IPFS] block_info:" << block_info_result.data.c_str()
      // << " \r\njson_value:" << json_value->DebugString();
      callback.Run(GetBlockFactory()->CreateCarBlock(
          block_info_result.cid.c_str(), std::move(json_value), nullptr, absl::nullopt));
      continue;
    }

    auto block_content = DecodeBlockContent(0, vec);

    if (block_content.error.error_code != 0) {
      LOG(INFO) << "[IPFS] Could not decode block!!! error:"
                << block_info_result.error.error.c_str();
      return;
    }
    const auto verified = absl::make_optional<bool>(block_content.verified);
    callback.Run(GetBlockFactory()->CreateCarBlock(
        block_content.cid.c_str(), base::Value(),
        std::make_unique<std::vector<uint8_t>>(block_content.data.begin(),
                                                block_content.data.end()),
        verified));

    // std::stringstream ss;
    // base::ranges::for_each(block_content.data,
    //                        [&](const uint8_t& item) { ss << (char)item; });
    // LOG(INFO) << "[IPFS] block_content:" << ss.str();
  }
}

}  // namespace ipfs::ipld
