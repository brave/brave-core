/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto_util.h"

#include "bat/ads/internal/logging.h"
#include "wrapper.hpp"

namespace ads {
namespace privacy {
namespace cbr {

using challenge_bypass_ristretto::TokenException;

bool ExceptionOccurred() {
  const TokenException e = challenge_bypass_ristretto::get_last_exception();
  if (!e.is_empty()) {
    BLOG(0, "Challenge Bypass Ristretto Error: " << e.what());
    return true;
  }

  return false;
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
