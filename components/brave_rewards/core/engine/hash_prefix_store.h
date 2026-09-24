/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_HASH_PREFIX_STORE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_HASH_PREFIX_STORE_H_

#include <memory>
#include <string>
#include <string_view>

#include "base/files/file_path.h"
#include "base/files/memory_mapped_file.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_rewards/core/mojom/rewards_database.mojom.h"

namespace brave_rewards::internal {

// Responsible for storage and retrieval of a sorted hash prefix list. The
// operations of this class will block the current thread on IO.
class HashPrefixStore : public mojom::HashPrefixStore {
 public:
  explicit HashPrefixStore(base::FilePath path);
  ~HashPrefixStore() override;

  HashPrefixStore(const HashPrefixStore&) = delete;
  HashPrefixStore& operator=(const HashPrefixStore&) = delete;

  // Opens the hash prefix file, if not already open.
  bool Open();

  // Closes the hash prefix file, if open.
  void Close();

  // Updates the hash prefix file.
  bool UpdatePrefixes(const std::string& prefixes, size_t prefix_size);

  // Returns a value indicating if the specified value exists in the prefix
  // list. Opens the file if not already open.
  bool ContainsPrefix(const std::string& value);

  // mojom::HashPrefixStore:
  void UpdatePrefixes(mojom::HashPrefixDataPtr prefix_data,
                      UpdatePrefixesCallback callback) override;

  void ContainsPrefix(const std::string& value,
                      ContainsPrefixCallback callback) override;

 private:
  const base::FilePath file_path_;
  std::unique_ptr<base::MemoryMappedFile> mapped_file_;
  std::string_view prefixes_;
  size_t prefix_size_ = 0;
  bool open_called_ = false;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_HASH_PREFIX_STORE_H_
