/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/token_generator.h"

namespace brave_ads {

cbr::TokenList TokenGenerator::Generate(const size_t count) const {
  return cbr::TokenList(count);
}

}  // namespace brave_ads
