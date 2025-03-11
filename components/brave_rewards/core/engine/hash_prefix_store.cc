/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/hash_prefix_store.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/files/file.h"
#include "brave/components/brave_rewards/core/engine/hash_prefix_iterator.h"
#include "brave/components/brave_rewards/core/engine/publisher/flat/prefix_storage_generated.h"
#include "crypto/sha2.h"
#include "third_party/flatbuffers/src/include/flatbuffers/flatbuffer_builder.h"
#include "third_party/flatbuffers/src/include/flatbuffers/vector.h"
#include "third_party/flatbuffers/src/include/flatbuffers/verifier.h"

namespace brave_rewards::internal {

namespace {

constexpr uint32_t kFlatStorageMagic = 0xBEEF0001;
constexpr size_t kOffsetsSize = 256 * 256;

constexpr uint32_t kMinPrefixSize = 4;
constexpr uint32_t kMaxPrefixSize = 32;

bool IsValidPrefixSize(size_t prefix_size) {
  return prefix_size >= kMinPrefixSize && prefix_size <= kMaxPrefixSize;
}

std::optional<size_t> GetPrefixCount(size_t byte_length, size_t prefix_size) {
  if (!IsValidPrefixSize(prefix_size)) {
    return std::nullopt;
  }
  if (byte_length % prefix_size != 0) {
    return std::nullopt;
  }
  return byte_length / prefix_size;
}

std::pair<uint16_t, std::string_view> GetIndexAndSuffix(
    const std::string_view& hash) {
  CHECK(hash.size() >= 2);
  const uint16_t first_bytes =
      256u * static_cast<uint8_t>(hash[0]) + static_cast<uint8_t>(hash[1]);
  return std::make_pair(first_bytes, hash.substr(2));
}

}  // namespace

HashPrefixStore::HashPrefixStore(base::FilePath path)
    : file_path_(std::move(path)) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

HashPrefixStore::~HashPrefixStore() = default;

bool HashPrefixStore::Open() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (mapped_file_) {
    DCHECK(mapped_file_->IsValid());
    return true;
  }

  open_called_ = true;

  auto mapped_file = std::make_unique<base::MemoryMappedFile>();
  if (!mapped_file->Initialize(file_path_)) {
    return false;
  }

  const auto bytes = mapped_file->bytes();

  auto verifier = flatbuffers::Verifier(bytes.data(), bytes.size());
  if (!rewards::flat::VerifyPrefixStorageBuffer(verifier)) {
    return false;
  }

  const auto* storage = rewards::flat::GetPrefixStorage(bytes.data());
  if (!storage || storage->magic() != kFlatStorageMagic ||
      !IsValidPrefixSize(storage->prefix_size())) {
    return false;
  }

  if (!storage->offsets() || storage->offsets()->size() != kOffsetsSize) {
    return false;
  }

  if (!storage->all_suffixes() || storage->all_suffixes()->size() == 0 ||
      storage->all_suffixes()->size() % (storage->prefix_size() - 2) != 0) {
    return false;
  }

  mapped_file_ = std::move(mapped_file);
  return true;
}

void HashPrefixStore::Close() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  open_called_ = false;
  mapped_file_.reset();
}

bool HashPrefixStore::UpdatePrefixes(const std::string& prefixes,
                                     size_t prefix_size) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  Close();

  auto prefix_count = GetPrefixCount(prefixes.length(), prefix_size);
  if (!prefix_count) {
    return false;
  }

  flatbuffers::FlatBufferBuilder builder;

  std::vector<std::vector<char>> suffix_arrays(kOffsetsSize);
  std::array<uint32_t, kOffsetsSize> offsets = {};

  const std::string_view prefixes_view(prefixes);

  for (size_t i = 0; i < prefix_count.value(); i++) {
    const auto prefix = prefixes_view.substr(i * prefix_size, prefix_size);
    const auto [index, suffix] = GetIndexAndSuffix(prefix);
    suffix_arrays[index].insert(suffix_arrays[index].end(), suffix.begin(),
                                suffix.end());
  }

  // Put all suffixes into a single vector and calculate the offsets.
  std::vector<char> all_suffixes_data;
  uint32_t current_offset = 0;

  for (size_t i = 0; i < suffix_arrays.size(); i++) {
    offsets[i] = current_offset;
    const auto& array = suffix_arrays[i];
    all_suffixes_data.insert(all_suffixes_data.end(), array.begin(),
                             array.end());
    current_offset += array.size();
  }

  // Serialize the vectors and the main table.
  auto flat_offsets = builder.CreateVector(offsets.data(), offsets.size());
  auto all_suffixes = builder.CreateVector(
      reinterpret_cast<const uint8_t*>(all_suffixes_data.data()),
      all_suffixes_data.size());
  auto prefix_storage = rewards::flat::CreatePrefixStorage(
      builder, kFlatStorageMagic, prefix_size, flat_offsets, all_suffixes);

  builder.Finish(prefix_storage);

  base::File file;
  file.Initialize(file_path_,
                  base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  if (!file.IsValid()) {
    return false;
  }

  const bool success =
      file.WriteAtCurrentPos(builder.GetBufferSpan()) == builder.GetSize();

  file.Close();
  return success;
}

bool HashPrefixStore::ContainsPrefix(const std::string& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!open_called_) {
    Open();
  }

  if (!mapped_file_) {
    return false;
  }

  const auto* storage = rewards::flat::GetPrefixStorage(mapped_file_->data());

  // Already validated in Open().
  CHECK(storage);
  CHECK(storage->offsets());
  CHECK(storage->all_suffixes());

  const auto offsets = flatbuffers::make_span(storage->offsets());
  const auto all_suffixes = flatbuffers::make_span(storage->all_suffixes());

  const auto prefix_size = storage->prefix_size();
  const size_t suffix_size = prefix_size - 2;

  auto hash = crypto::SHA256HashString(value);
  hash.resize(prefix_size);

  const auto [index, suffix] = GetIndexAndSuffix(hash);

  if (index >= offsets.size()) {
    return false;
  }

  // Get the range to lookup this prefix in the all_suffixes array.
  const uint32_t start_idx = offsets[index];
  const uint32_t end_idx =
      (index < offsets.size() - 1) ? offsets[index + 1] : all_suffixes.size();

  if (start_idx >= end_idx || start_idx >= all_suffixes.size() ||
      end_idx > all_suffixes.size()) {
    return false;
  }

  const auto* data = reinterpret_cast<const char*>(all_suffixes.data());
  HashPrefixIterator begin(data, start_idx / suffix_size, suffix_size);
  HashPrefixIterator end(data, end_idx / suffix_size, suffix_size);

  return std::binary_search(begin, end, suffix);
}

}  // namespace brave_rewards::internal
