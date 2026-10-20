// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/core/sync_code.h"

#include <algorithm>
#include <vector>

#include "base/containers/span.h"
#include "base/strings/string_split.h"
#include "brave/components/brave_sync/crypto/crypto.h"

namespace brave_transfer {

namespace {

constexpr size_t kSyncCodeWords = 24;

}  // namespace

std::optional<Key> SeedFromSyncCode(const std::string& sync_code) {
  std::vector<std::string_view> words = base::SplitStringPiece(
      sync_code, " ", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  if (words.size() != kSyncCodeWords ||
      std::ranges::any_of(words,
                          [](std::string_view word) { return word.empty(); })) {
    return std::nullopt;
  }
  std::vector<uint8_t> bytes;
  if (!brave_sync::crypto::PassphraseToBytes32(sync_code, &bytes) ||
      bytes.size() != kKeySize) {
    return std::nullopt;
  }
  Key seed;
  base::span(seed).copy_from(bytes);
  return seed;
}

}  // namespace brave_transfer
