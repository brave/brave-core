/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"
// This is done to allow the same renaming in
// chromium_src/chrome/browser/prefs/browser_prefs.cc
#include "components/translate/core/browser/translate_prefs.h"

#define MigrateObsoleteProfilePrefs MigrateObsoleteProfilePrefs_ChromiumImpl
#include "../../../../../../components/translate/core/browser/translate_prefs.cc"
#undef MigrateObsoleteProfilePrefs

namespace translate {

OVERRIDE_FEATURE_DEFAULT_STATES({{
#if defined(OS_ANDROID)
    {kTranslate, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
}});

}  // namespace translate
