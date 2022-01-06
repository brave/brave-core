/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_ENUM_STRING_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_ENUM_STRING_H_

#include <string>
#include <vector>

#include "base/strings/string_piece.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ledger {

template <typename T>
class EnumString : public base::StringPiece {
 public:
  explicit EnumString(const std::string& str) : base::StringPiece(str) {}

  inline static absl::optional<T> Parse(const std::string& str) {
    return ParseEnum(EnumString(str));
  }

  absl::optional<T> Match(const std::vector<T>& values) const {
    for (auto v : values) {
      if (*this == StringifyEnum(v)) {
        return v;
      }
    }
    return {};
  }
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_ENUM_STRING_H_
