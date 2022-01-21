/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_PREF_NAMES_H_
#define BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_PREF_NAMES_H_

namespace de_amp {

// Is De-AMP feature currently enabled
constexpr char kDeAmpPrefEnabled[] = "brave.de_amp.enabled";

// Set if DeAmp was enabled at least once
constexpr char kDeAmpPrefEverEnabled[] = "brave.de_amp.ever_enabled";

// Number of times the user has toggled DeAmp
constexpr char kDeAmpPrefToggleCount[] = "brave.de_amp.toggle_count";

// Number of times the "Enable DeAmp" button was shown automatically
constexpr char kDeAmpPrefPromptCount[] = "brave.de_amp.prompt_count";

}  // namespace de_amp

#endif  // BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_PREF_NAMES_H_
