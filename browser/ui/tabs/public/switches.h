/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_PUBLIC_SWITCHES_H_
#define BRAVE_BROWSER_UI_TABS_PUBLIC_SWITCHES_H_

namespace tabs::switches {

// This switch disables vertical tab strip regardless of the pref. This could be
// useful when vertical tab strip causes browser to crash on start up.
inline constexpr char kDisableVerticalTabsSwitch[] = "disable-vertical-tabs";

// This switch should be followed by a number in milliseconds, which
// specifies the delay before expanding the vertical tab strip when hovering
// over it.
inline constexpr char kVerticalTabExpandDelaySwitch[] =
    "vertical-tab-expand-delay";

// This switch should be followed by a number in milliseconds, which
// specifies the delay before collapsing the vertical tab strip when the mouse
// exits the tab strip in floating mode.
inline constexpr char kVerticalTabCollapseDelaySwitch[] =
    "vertical-tab-collapse-delay";

// Testing-only switch for the Brave -> upstream vertical tabs migration.
// "force-upstream" routes to Chromium's native vertical tabs; "reset" clears
// any forced override and reverts to whatever the pref would otherwise be
// (i.e. Brave's own vertical tabs, since the upstream pref defaults false).
inline constexpr char kVerticalTabMigrationSwitch[] = "vertical-tab-migration";
inline constexpr char kVerticalTabMigrationForceUpstreamValue[] =
    "force-upstream";
inline constexpr char kVerticalTabMigrationResetValue[] = "reset";

}  // namespace tabs::switches

#endif  // BRAVE_BROWSER_UI_TABS_PUBLIC_SWITCHES_H_
