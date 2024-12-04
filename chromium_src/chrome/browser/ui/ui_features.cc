// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "src/chrome/browser/ui/ui_features.cc"

#include "base/feature_list.h"
#include "base/feature_override.h"

namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
    {kFewerUpdateConfirmations, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
    {kTabHoverCardImages, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace features
