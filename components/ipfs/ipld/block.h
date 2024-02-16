/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_IPLD_BLOCK_H_
#define BRAVE_COMPONENTS_IPFS_IPLD_BLOCK_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "absl/types/optional.h"
#include "base/values.h"

namespace ipfs::ipld {

struct DJLink {
  std::string hash;
  std::string name;
  uint64_t size;
};

class Block {
 public:
  explicit Block(const std::string& cid,
                 base::Value::Dict metadata,
                 std::unique_ptr<std::vector<uint8_t>> data,
                 std::unique_ptr<std::vector<DJLink>> djlinks,
                 std::unique_ptr<std::vector<uint8_t>> djdata,
                 const absl::optional<bool>& verified);
  ~Block();

  const std::string Cid() const;

  bool IsRoot() const;
  bool IsMetadata() const;
  bool IsContent() const;
  absl::optional<bool> IsVerified() const;

  const base::Value::Dict& Meta() const;
  const std::vector<uint8_t>* GetContentData() const;

  const std::vector<DJLink>* GetLinks() const;
  const std::vector<uint8_t>* GetData() const;

 private:
  std::string cid_;  
  base::Value::Dict metadata_;
  std::unique_ptr<std::vector<uint8_t>> data_;
  std::unique_ptr<std::vector<DJLink>> djlinks_;
  std::unique_ptr<std::vector<uint8_t>> djdata_;
  absl::optional<bool> verified_;
};

class BlockFactory {
 public:
  BlockFactory() = default;
  ~BlockFactory() = default;

  std::unique_ptr<Block> CreateCarBlock(
      const std::string& cid,
      absl::optional<base::Value> metadata,
      std::unique_ptr<std::vector<uint8_t>> data,
      const absl::optional<bool> verified);
};

}  // namespace ipfs::ipld

#endif  // BRAVE_COMPONENTS_IPFS_IPLD_BLOCK_H_
