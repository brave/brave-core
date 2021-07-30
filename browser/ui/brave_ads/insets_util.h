/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_ADS_INSETS_UTIL_H_
#define BRAVE_BROWSER_UI_BRAVE_ADS_INSETS_UTIL_H_

namespace gfx {
class FontList;
class Insets;
}  // namespace gfx

namespace brave_ads {

void AdjustInsetsForFontList(gfx::Insets* insets,
                             const gfx::FontList& font_list);

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_BRAVE_ADS_INSETS_UTIL_H_
