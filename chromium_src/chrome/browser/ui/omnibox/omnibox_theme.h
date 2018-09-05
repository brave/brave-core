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

// re-define original enum
#define OmniboxTint OmniboxTint_Chromium
#define GetOmniboxColor GetOmniboxColor_ChromiumImpl
#include "../../../chrome/browser/ui/omnibox/omnibox_theme.h"
#undef OmniboxTint
#undef GetOmniboxColor

// define new enum
enum class OmniboxTint { DARK, LIGHT, NATIVE, PRIVATE };

// Returns the color for the given |part| and |tint|. An optional |state| can be
// provided for OmniboxParts that support stateful colors.
SkColor GetOmniboxColor(OmniboxPart part,
                        OmniboxTint tint,
                        OmniboxPartState state = OmniboxPartState::NORMAL);

#endif
