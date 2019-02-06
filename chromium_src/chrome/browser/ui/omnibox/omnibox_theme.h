/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_THEME_H_OVERRIDE_
#define CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_THEME_H_OVERRIDE_

// Intention is to add a value to OmniboxTint since we want to support more
// states than Chromium but do not want to override / patch every call
// to GetOmniboxColor.
// We'd rather override LocationBarView::GetTint to return the extra enum value
// and override GetOmniboxColor to support it. So that means allowing the many
// places which pass around the enum value to support our enum values instead.

// define new enum
enum class OmniboxTint { DARK, LIGHT, NATIVE, PRIVATE };

#include "../../../chrome/browser/ui/omnibox/omnibox_theme.h"

#endif  // CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_THEME_H_OVERRIDE_
