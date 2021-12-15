/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/unblinded_tokens_frequency_cap_unittest_util.h"

#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens_unittest_util.h"

namespace ads {

void ForceUnblindedTokensFrequencyCapPermission() {
  privacy::SetUnblindedTokens(10);
}

}  // namespace ads
