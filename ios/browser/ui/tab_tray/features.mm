// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/tab_tray/features.h"

namespace brave::features {

BASE_FEATURE(kModernTabTrayEnabled,
             "ModernTabTrayEnabled",
             base::FEATURE_DISABLED_BY_DEFAULT);
}
