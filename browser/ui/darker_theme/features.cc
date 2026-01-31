// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/darker_theme/features.h"

namespace darker_theme::features {

// Note that we're exposing the feature with the name "BraveUltraDarkTheme" for
// brand consistency
BASE_FEATURE(kBraveDarkerTheme,
             "BraveUltraDarkTheme",
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace darker_theme::features
