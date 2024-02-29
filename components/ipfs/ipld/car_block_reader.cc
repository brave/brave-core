/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/car_block_reader.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "absl/types/optional.h"
#include "base/containers/span.h"
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

bool ProcessHeader(const std::vector<uint8_t>& block_data,
                   bool& is_header_retrieved,
                   ipfs::ipld::BlockReader::BlockReaderCallback const& callback,
                   ipfs::ipld::BlockFactory* block_factory) {
  auto carv1_header_result = ipfs::ipld::DecodeCarv1Header(block_data);
  DCHECK(carv1_header_result.error.error_code == 0)
      << carv1_header_result.error.error.c_str();
  if (carv1_header_result.error.error_code != 0) {
    return false;
  }
  is_header_retrieved = true;

  base::Value::List roots_items;
  base::ranges::for_each(
      carv1_header_result.data.roots,
      [&roots_items](auto& item) { roots_items.Append(item.c_str()); });
  base::Value::Dict roots_dict;
  roots_dict.Set("roots", std::move(roots_items));

  callback.Run(block_factory->CreateCarBlock(
      "", base::Value(std::move(roots_dict)), nullptr, absl::nullopt), false);
  return true;
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
    const bool is_completed) {
  if (is_completed && !data) {
    is_header_retrieved_ = false;
    buffer_.clear();
    callback.Run(nullptr, true);
    return;
  }

  buffer_.insert(buffer_.end(), std::make_move_iterator(data->begin()),
                 std::make_move_iterator(data->end()));
  data.reset();

  base::span<const uint8_t> buffer_span(buffer_);

  while (!buffer_span.empty()) {
    uint64_t received_buffer_size = buffer_span.size();
    if (received_buffer_size < sizeof(uint64_t)) {
      return;
    }

    const uint64_t current_block_size = DecodeBlockLength(buffer_span);
    if (received_buffer_size < current_block_size) {
      return;
    }
    const size_t block_length_bytes = buffer_.size() - buffer_span.size();

    base::span block_span = buffer_span.subspan(0, current_block_size);
    buffer_span = buffer_span.subspan(current_block_size);

    const std::vector<uint8_t> block_data(block_span.begin(), block_span.end());
    buffer_.erase(
        buffer_.begin(),
        std::next(buffer_.begin(), current_block_size + block_length_bytes));
    buffer_span = base::span<const uint8_t>(buffer_);

    if (!is_header_retrieved_) {
      if (!ProcessHeader(block_data, is_header_retrieved_, callback,
                         GetBlockFactory())) {
        return;
      }
      continue;
    }

    auto block_info_result = DecodeBlockInfo(0, block_data);
    DCHECK(block_info_result.error.error_code == 0) << block_info_result.error.error.c_str();
    if (block_info_result.error.error_code != 0) {
      return;
    }

    if (!block_info_result.is_content) {
      auto json_value = ParseJsonHelper(block_info_result.data.c_str(),
                                        base::Value::Type::DICT);
      callback.Run(GetBlockFactory()->CreateCarBlock(
          block_info_result.cid.c_str(), std::move(json_value), nullptr,
          absl::nullopt), false);
      continue;
    }

    auto block_content = DecodeBlockContent(0, block_data);
    DCHECK(block_content.error.error_code == 0) << block_content.error.error.c_str();
    if (block_content.error.error_code != 0) {
      return;
    }
    const auto verified = absl::make_optional<bool>(block_content.verified);
    callback.Run(GetBlockFactory()->CreateCarBlock(
        block_content.cid.c_str(), base::Value(),
        std::make_unique<std::vector<uint8_t>>(block_content.data.begin(),
                                               block_content.data.end()),
        verified), false);
  }
}

}  // namespace ipfs::ipld
