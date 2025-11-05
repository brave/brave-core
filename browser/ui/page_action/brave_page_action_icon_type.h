/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_PAGE_ACTION_BRAVE_PAGE_ACTION_ICON_TYPE_H_
#define BRAVE_BROWSER_UI_PAGE_ACTION_BRAVE_PAGE_ACTION_ICON_TYPE_H_

#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "chrome/browser/ui/page_action/page_action_icon_type.h"

namespace brave {

#define DECLARE_BRAVE_PAGE_ACTION_ICON_TYPE(NAME, VALUE) \
  constexpr PageActionIconType NAME = static_cast<PageActionIconType>(VALUE)

// Use negative values so that our values doesn't conflict with upstream values.
DECLARE_BRAVE_PAGE_ACTION_ICON_TYPE(kUndefinedPageActionIconType, -1);
DECLARE_BRAVE_PAGE_ACTION_ICON_TYPE(kPlaylistPageActionIconType, -2);
// -3 was used for Brave Player
#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
DECLARE_BRAVE_PAGE_ACTION_ICON_TYPE(kWaybackMachineActionIconType, -4);
#endif
#if BUILDFLAG(ENABLE_SPEEDREADER)
DECLARE_BRAVE_PAGE_ACTION_ICON_TYPE(kSpeedreaderPageActionIconType, -5);
#endif

#undef DECLARE_BRAVE_PAGE_ACTION_ICON_TYPE

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_PAGE_ACTION_BRAVE_PAGE_ACTION_ICON_TYPE_H_
