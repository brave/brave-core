// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_recap/common/features.h"

namespace brave_recap::features {

// Enables brave://recap, a shareable summary of lifetime Shields stats.
BASE_FEATURE(kBraveRecap, base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace brave_recap::features
