/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_TALK_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_TALK_PREF_NAMES_H_

#include "brave/components/brave_talk/buildflags/buildflags.h"

// Ensure this header is only included when Brave Talk is enabled
static_assert(BUILDFLAG(ENABLE_BRAVE_TALK),
              "brave_talk/pref_names.h should not be included when "
              "ENABLE_BRAVE_TALK is false");

namespace brave_talk {
namespace prefs {

inline constexpr char kNewTabPageShowBraveTalk[] =
    "brave.new_tab_page.show_together";

inline constexpr char kDisabledByPolicy[] = "brave.talk.disabled_by_policy";

}  // namespace prefs
}  // namespace brave_talk

#endif  // BRAVE_COMPONENTS_BRAVE_TALK_PREF_NAMES_H_
