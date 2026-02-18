/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/features.h"

#include "base/feature_list.h"

namespace brave::features {

// Use base::WeakPtr instead of std::shared_ptr for BraveRequestInfo.
// When enabled, BraveRequestInfo objects are owned by std::unique_ptr
// and non-owning references use base::WeakPtr for automatic null checking.
// When disabled (default), std::shared_ptr is used for both ownership
// and non-owning references (simpler but higher overhead).
BASE_FEATURE(kBraveRequestInfoUseWeakPtr, base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace brave::features
