/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/hd_key_utils.h"

#include <optional>

#include "base/containers/span.h"
#include "base/numerics/checked_math.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"

namespace brave_wallet {

std::optional<std::vector<uint32_t>> ParseFullHDPath(std::string_view path) {
  auto entries = base::SplitStringPiece(
      path, "/", base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_ALL);

  if (entries.empty()) {
    return std::nullopt;
  }
  if (entries.front() != kMasterNode) {
    return std::nullopt;
  }

  std::vector<uint32_t> result;
  result.reserve(entries.size() - 1);
  for (auto node : base::span(entries).subspan(1)) {
    uint32_t offset = 0;
    if (node.ends_with('\'')) {
      node.remove_suffix(1);
      offset = kHardenedOffset;
    }
    uint32_t value = 0;
    if (!base::StringToUint(node, &value)) {
      return std::nullopt;
    }
    if (value >= kHardenedOffset) {
      return std::nullopt;
    }
    result.push_back(base::CheckAdd(value, offset).ValueOrDie<uint32_t>());
  }

  return result;
}

}  // namespace brave_wallet
