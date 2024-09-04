/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ACCOUNT_CONFIRMATIONS_CONFIRMATION_TYPE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ACCOUNT_CONFIRMATIONS_CONFIRMATION_TYPE_H_

#include <string_view>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

// Returns a `mojom::ConfirmationType` value based on the string input.
mojom::ConfirmationType ToMojomConfirmationType(std::string_view value);

// Returns a string constant for a given `mojom::ConfirmationType` value.
const char* ToString(mojom::ConfirmationType mojom_confirmation_type);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ACCOUNT_CONFIRMATIONS_CONFIRMATION_TYPE_H_
