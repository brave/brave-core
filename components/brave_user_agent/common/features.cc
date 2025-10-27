// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_user_agent/common/features.h"

#include "base/feature_list.h"

namespace brave_user_agent::features {

BASE_FEATURE(kUseBraveUserAgent,
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace brave_user_agent::features
