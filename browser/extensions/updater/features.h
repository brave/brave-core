// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_EXTENSIONS_UPDATER_FEATURES_H_
#define BRAVE_BROWSER_EXTENSIONS_UPDATER_FEATURES_H_

#include "base/feature_list.h"

namespace brave_extensions::features {

// When enabled (default), user installed extensions are automatically updated
// via component updater on a regular frequency.
//
// Can be disabled to prevent automatic extension updates. Folks can still
// update them manually from the brave://extensions page.
BASE_DECLARE_FEATURE(kBraveExtensionAutoUpdate);

}  // namespace brave_extensions::features

#endif  // BRAVE_BROWSER_EXTENSIONS_UPDATER_FEATURES_H_
