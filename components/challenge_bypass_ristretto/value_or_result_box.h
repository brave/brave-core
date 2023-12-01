/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_VALUE_OR_RESULT_BOX_H_
#define BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_VALUE_OR_RESULT_BOX_H_

#include <utility>

#include "base/check.h"
#include "base/functional/overloaded.h"
#include "third_party/abseil-cpp/absl/types/variant.h"

namespace challenge_bypass_ristretto {

template <typename TValue, typename TValueBox, typename TResultBox>
class ValueOrResultBox {
 public:
  explicit ValueOrResultBox(TValueBox box) : box_(std::move(box)) {}
  explicit ValueOrResultBox(TResultBox box) : box_(std::move(box)) {}

  ValueOrResultBox(const ValueOrResultBox&) = delete;
  ValueOrResultBox& operator=(const ValueOrResultBox&) = delete;

  ValueOrResultBox(ValueOrResultBox&&) noexcept = default;
  ValueOrResultBox& operator=(ValueOrResultBox&&) noexcept = default;

  const TValue& unwrap() const {
    return absl::visit(
        base::Overloaded{
            [](const TValueBox& box) -> const TValue& { return *box; },
            [](const TResultBox& box) -> const TValue& {
              CHECK(box->is_ok());
              return box->unwrap();
            }},
        box_);
  }

 private:
  absl::variant<TValueBox, TResultBox> box_;
};

}  // namespace challenge_bypass_ristretto

#endif  // BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_VALUE_OR_RESULT_BOX_H_
