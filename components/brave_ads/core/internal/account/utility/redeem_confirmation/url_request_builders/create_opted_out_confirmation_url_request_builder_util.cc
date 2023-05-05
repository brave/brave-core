/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/url_request_builders/create_opted_out_confirmation_url_request_builder_util.h"

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_ads/core/internal/account/utility/confirmation_constants.h"

namespace brave_ads {

std::string BuildCreateOptedOutConfirmationUrlPath(
    const std::string& transaction_id) {
  DCHECK(!transaction_id.empty());

  return base::StringPrintf("/v%d/confirmation/%s", kConfirmationServerVersion,
                            transaction_id.c_str());
}

}  // namespace brave_ads
