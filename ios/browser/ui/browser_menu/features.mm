// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/browser_menu/features.h"

namespace brave::features {

BASE_FEATURE(kModernBrowserMenuEnabled,
             "ModernBrowserMenuEnabled",
             base::FEATURE_ENABLED_BY_DEFAULT);
}
