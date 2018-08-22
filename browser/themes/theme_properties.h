/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_THEME_PROPERTIES_H_
#define BRAVE_BROWSER_THEMES_THEME_PROPERTIES_H_

#include "base/optional.h"
#include "third_party/skia/include/core/SkColor.h"

class Profile;

#define BRAVE_COLOR_FOR_TEST 0x7FFFFFFF

base::Optional<SkColor> MaybeGetDefaultColorForBraveUi(int id, bool incognito, Profile* profile);

#define MAYBE_OVERRIDE_DEFAULT_COLOR_FOR_BRAVE(id, incognito, profile) \
  const base::Optional<SkColor> braveColor = MaybeGetDefaultColorForBraveUi(id, incognito, profile); \
  if (braveColor) return braveColor.value();

#endif  // BRAVE_BROWSER_THEMES_THEME_PROPERTIES_H_
