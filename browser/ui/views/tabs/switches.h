/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_SWITCHES_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_SWITCHES_H_

namespace tabs::switches {

// This switch disables vertical tab strip regardless of the pref. This could be
// useful when vertical tab strip causes browser to crash on start up.
inline constexpr char kDisableVerticalTabsSwitch[] = "disable-vertical-tabs";

}  // namespace tabs::switches

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_SWITCHES_H_
