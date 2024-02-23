/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/block.h"

#include <cstdint>
#include <memory>
#include <utility>

#include "absl/types/optional.h"
#include "base/strings/string_number_conversions.h"

namespace {
constexpr char kRootBlocks[] = "roots";
constexpr char kDjLinks[] = "Links";
constexpr char kDjLinkName[] = "Name";
constexpr char kDjLinkTsize[] = "Tsize";
constexpr char kDjLinkHash[] = "Hash";
constexpr char kDjData[] = "Data";
constexpr char kDjDataDictSlash[] = "/";
constexpr char kDjDataDictBytes[] = "bytes";

std::unique_ptr<std::vector<ipfs::ipld::DJLink>> ParseLinksFromMeta(
    const base::Value::Dict& metadata) {
  if (metadata.FindList(kRootBlocks)) {
    return nullptr;
  }

  const auto* links = metadata.FindList(kDjLinks);
  DCHECK(links);
  if (!links) {
    return nullptr;
  }
  auto result = std::make_unique<std::vector<ipfs::ipld::DJLink>>();
  for (const auto& item : *links) {
    DCHECK(item.is_dict());
    if (!item.is_dict()) {
      continue;
    }

    const auto& curr_item_dict = item.GetDict();
    const auto* name = curr_item_dict.FindString(kDjLinkName);
    const auto size = curr_item_dict.FindDouble(kDjLinkTsize);
    const auto* hash_dict = curr_item_dict.FindDict(kDjLinkHash);

    DCHECK(hash_dict);
    if (!hash_dict) {
      continue;
    }

    const auto* link_to_block = hash_dict->FindString(kDjDataDictSlash);
    DCHECK(link_to_block);
    if (!link_to_block) {
      continue;
    }

    DCHECK(size.has_value());
    if (!size.has_value()) {
      continue;
    }

    ipfs::ipld::DJLink dj_link{*link_to_block, *name,
                               static_cast<uint64_t>(size.value())};

    result->emplace_back(std::move(dj_link));
  }

  return result->empty() ? nullptr : std::move(result);
}

std::unique_ptr<std::vector<uint8_t>> ParseDataFromMeta(
    const base::Value::Dict& metadata) {
  if (metadata.FindList(kRootBlocks)) {
    return nullptr;
  }

  const auto* data = metadata.FindDict(kDjData);
  DCHECK(data);
  if (!data) {
    return nullptr;
  }
  const auto* data_slash = data->FindDict(kDjDataDictSlash);
  DCHECK(data_slash);
  if (!data_slash) {
    return nullptr;
  }

  const auto* bytes_str = data_slash->FindString(kDjDataDictBytes);
  DCHECK(bytes_str);
  if (!bytes_str) {
    return nullptr;
  }
  auto result = std::make_unique<std::vector<uint8_t>>();
  base::HexStringToBytes(*bytes_str, &(*result));
  return result;
}
}  // namespace

namespace ipfs::ipld {
Block::Block(const std::string& cid,
             base::Value::Dict metadata,
             std::unique_ptr<std::vector<uint8_t>> data,
             std::unique_ptr<std::vector<DJLink>> djlinks,
             std::unique_ptr<std::vector<uint8_t>> djdata,
             const absl::optional<bool>& verified)
    : cid_(cid),
      metadata_(std::move(metadata)),
      data_(std::move(data)),
      djlinks_(std::move(djlinks)),
      djdata_(std::move(djdata)),
      verified_(verified) {}

Block::~Block() = default;

std::string Block::Cid() const {
  return cid_;
}

bool Block::IsRoot() const {
  return !metadata_.empty() && metadata_.FindList(kRootBlocks);
}

bool Block::IsMetadata() const {
  return !metadata_.empty() && !metadata_.FindList("roots") &&
         (metadata_.FindList(kDjLinks) || metadata_.FindList(kDjData));
}

bool Block::IsContent() const {
  return metadata_.empty() && data_ && !data_->empty();
}

absl::optional<bool> Block::IsVerified() const {
  return verified_;
}

const base::Value::Dict& Block::Meta() const {
  return metadata_;
}

const std::vector<uint8_t>* Block::GetContentData() const {
  return data_.get();
}

const std::vector<DJLink>* Block::GetLinks() const {
  return djlinks_.get();
}

const std::vector<uint8_t>* Block::GetData() const {
  return djdata_.get();
}

std::unique_ptr<Block> BlockFactory::CreateCarBlock(
    const std::string& cid,
    absl::optional<base::Value> metadata,
    std::unique_ptr<std::vector<uint8_t>> data,
    const absl::optional<bool>& verified) {
  if (!metadata.has_value() || !metadata->is_dict()) {
    return std::unique_ptr<Block>(new Block(cid, base::Value::Dict(), std::move(data),
                                   nullptr, nullptr, verified));
  }

  auto dj_links = ParseLinksFromMeta(metadata->GetDict());
  auto dj_data = ParseDataFromMeta(metadata->GetDict());

  return std::unique_ptr<Block>(new Block(cid, std::move(metadata->GetDict()),
                                 std::move(data), std::move(dj_links),
                                 std::move(dj_data), verified));
}

}  // namespace ipfs::ipld
