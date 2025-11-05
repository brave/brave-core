/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/features.h"

namespace brave_account::features {

BASE_FEATURE(kBraveAccount, base::FEATURE_DISABLED_BY_DEFAULT);

bool IsBraveAccountEnabled() {
  return base::FeatureList::IsEnabled(kBraveAccount);
}

}  // namespace brave_account::features
