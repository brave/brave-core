// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/brave_ui_features.h"

namespace features {

BASE_FEATURE(kBraveNtpSearchWidget,
             "BraveNtpSearchWidget",
             base::FEATURE_ENABLED_BY_DEFAULT);

#if BUILDFLAG(IS_WIN)
// Enables window cloaking on window creation to prevent a white flash.
BASE_FEATURE(kBraveWorkaroundNewWindowFlash,
             "BraveWorkaroundNewWindowFlash",
             base::FEATURE_ENABLED_BY_DEFAULT);
#endif  // BUILDFLAG(IS_WIN)

}  // namespace features
