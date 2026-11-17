/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_PEEK_PEEK_PREFS_H_
#define BRAVE_BROWSER_UI_PEEK_PEEK_PREFS_H_

class PrefRegistrySimple;

namespace peek::prefs {

// The size the user resized the Peek card to, as fractions of the width and
// height of the contents area it covers, so it fits any window. 0 while the
// user hasn't resized it, so the card has its default size.
inline constexpr char kCardWidthRatio[] = "brave.peek.card_width_ratio";
inline constexpr char kCardHeightRatio[] = "brave.peek.card_height_ratio";

void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace peek::prefs

#endif  // BRAVE_BROWSER_UI_PEEK_PEEK_PREFS_H_
