/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/hash_prefix_store.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/containers/span_reader.h"
#include "base/files/file.h"
#include "base/memory/raw_span.h"
#include "base/numerics/byte_conversions.h"
#include "base/sequence_checker.h"
#include "base/strings/string_view_util.h"
#include "brave/components/brave_rewards/core/engine/hash_prefix_iterator.h"
#include "crypto/sha2.h"

namespace brave_rewards::internal {

namespace {

constexpr uint32_t kFileVersion = 1;
constexpr uint32_t kMinPrefixSize = 4;
constexpr uint32_t kMaxPrefixSize = 32;

struct FileParts {
  uint32_t version = 0;
  uint32_t prefix_size = 0;
  uint64_t prefix_count = 0;
  base::raw_span<const uint8_t> prefixes;
};

bool IsValidPrefixSize(size_t prefix_size) {
  return prefix_size >= kMinPrefixSize && prefix_size <= kMaxPrefixSize;
}

std::optional<size_t> GetPrefixCount(size_t byte_length, size_t prefix_size) {
  if (prefix_size == 0) {
    return std::nullopt;
  }
  if (byte_length % prefix_size != 0) {
    return std::nullopt;
  }
  return byte_length / prefix_size;
}

bool IsValidFile(const FileParts& parts) {
  if (parts.version != kFileVersion) {
    return false;
  }
  if (!IsValidPrefixSize(parts.prefix_size)) {
    return false;
  }
  auto count = GetPrefixCount(parts.prefixes.size(), parts.prefix_size);
  if (!count || count.value() != parts.prefix_count) {
    return false;
  }
  return true;
}

std::optional<FileParts> ParseFile(base::span<const uint8_t> bytes) {
  FileParts parts;
  base::SpanReader reader(bytes);
  if (!reader.ReadU32LittleEndian(parts.version)) {
    return std::nullopt;
  }
  if (!reader.ReadU32LittleEndian(parts.prefix_size)) {
    return std::nullopt;
  }
  if (!reader.ReadU64LittleEndian(parts.prefix_count)) {
    return std::nullopt;
  }
  parts.prefixes = reader.remaining_span();
  if (!IsValidFile(parts)) {
    return std::nullopt;
  }
  return parts;
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

  auto parts = ParseFile(mapped_file->bytes());
  if (!parts) {
    return false;
  }

  mapped_file_ = std::move(mapped_file);
  prefixes_ = base::as_string_view(parts->prefixes);
  prefix_size_ = parts->prefix_size;

  return true;
}

void HashPrefixStore::Close() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  prefixes_ = {};
  prefix_size_ = 0;
  open_called_ = false;
  mapped_file_.reset();
}

bool HashPrefixStore::UpdatePrefixes(const std::string& prefixes,
                                     size_t prefix_size) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  Close();

  if (!IsValidPrefixSize(prefix_size)) {
    return false;
  }

  auto prefix_count = GetPrefixCount(prefixes.length(), prefix_size);
  if (!prefix_count) {
    return false;
  }

  base::File file;
  file.Initialize(file_path_,
                  base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  if (!file.IsValid()) {
    return false;
  }

  auto encoded_version = base::U32ToLittleEndian(kFileVersion);
  if (!file.WriteAtCurrentPosAndCheck(base::as_byte_span(encoded_version))) {
    return false;
  }

  auto encoded_size =
      base::U32ToLittleEndian(base::checked_cast<uint32_t>(prefix_size));
  if (!file.WriteAtCurrentPosAndCheck(base::as_byte_span(encoded_size))) {
    return false;
  }

  auto encoded_count = base::U64ToLittleEndian(
      base::checked_cast<uint64_t>(prefix_count.value()));
  if (!file.WriteAtCurrentPosAndCheck(base::as_byte_span(encoded_count))) {
    return false;
  }

  if (!file.WriteAtCurrentPosAndCheck(base::as_byte_span(prefixes))) {
    return false;
  }

  file.Close();
  return true;
}

bool HashPrefixStore::ContainsPrefix(const std::string& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!open_called_) {
    Open();
  }

  if (prefix_size_ == 0) {
    return false;
  }

  std::string hash = crypto::SHA256HashString(value);
  hash.resize(prefix_size_);

  size_t prefix_count = prefixes_.length() / prefix_size_;

  return std::binary_search(
      HashPrefixIterator(prefixes_, 0, prefix_size_),
      HashPrefixIterator(prefixes_, prefix_count, prefix_size_), hash);
}

void HashPrefixStore::UpdatePrefixes(mojom::HashPrefixDataPtr prefix_data,
                                     UpdatePrefixesCallback callback) {
  std::move(callback).Run(
      UpdatePrefixes(prefix_data->prefixes, prefix_data->prefix_size));
}

void HashPrefixStore::ContainsPrefix(const std::string& value,
                                     ContainsPrefixCallback callback) {
  std::move(callback).Run(ContainsPrefix(value));
}

}  // namespace brave_rewards::internal
