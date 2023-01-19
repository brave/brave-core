/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_util.h"

#include "bat/ads/internal/common/logging_util.h"
#include "wrapper.hpp"

namespace ads::privacy::cbr {

bool ExceptionOccurred() {
  const challenge_bypass_ristretto::TokenException e =
      challenge_bypass_ristretto::get_last_exception();
  if (!e.is_empty()) {
    BLOG(0, "Challenge Bypass Ristretto Error: " << e.what());
    return true;
  }

  return false;
}

}  // namespace ads::privacy::cbr
