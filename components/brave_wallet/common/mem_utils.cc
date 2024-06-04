/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/mem_utils.h"

#include "base/ranges/algorithm.h"

namespace brave_wallet {

void SecureZeroData(base::span<uint8_t> bytes) {
  // 'volatile' is required. Otherwise optimizers can remove this function
  // if cleaning local variables, which are not used after that.
  base::span<volatile uint8_t> data = bytes;
  base::ranges::fill(data, 0u);
}

}  // namespace brave_wallet
