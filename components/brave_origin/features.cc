/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/features.h"

#include "base/feature_list.h"
#include "build/build_config.h"

namespace brave_origin::features {

BASE_FEATURE(kBraveOrigin, "BraveOrigin", base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace brave_origin::features
