// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_THEMES_SWITCHES_H_
#define BRAVE_BROWSER_THEMES_SWITCHES_H_

namespace switches {

// Sets the browser color scheme for all running Chrome windows.
// Valid values are "system", "light", or "dark".
inline constexpr char kSetColorScheme[] = "set-color-scheme";

// Sets the browser color variant preference.
// Valid values are "tonal_spot", "neutral", "vibrant", "expressive".
inline constexpr char kSetColorVariant[] = "set-color-variant";

// Resets to the default theme for all running Chrome windows.
inline constexpr char kSetDefaultTheme[] = "set-default-theme";

// Enables grayscale theme for all running Chrome windows.
// This is a boolean flag - presence enables grayscale, absence disables it.
inline constexpr char kSetGrayscaleTheme[] = "set-grayscale-theme";

// Sets the user color for Chrome Refresh theming.
// The format is "r,g,b", where r, g, b are numeric values from 0 to 255.
inline constexpr char kSetUserColor[] = "set-user-color";

}  // namespace switches

#endif  // BRAVE_BROWSER_THEMES_SWITCHES_H_
