// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/extensions/updater/features.h"

namespace extensions::features {

// When enabled (default), user installed extensions are automatically updated
// via component updater on a regular frequency.
//
// Can be disabled to prevent automatic extension updates. Folks can still
// update them manually from the brave://extensions page.
BASE_FEATURE(kBraveAutoUpdateExtensions,
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace extensions::features
