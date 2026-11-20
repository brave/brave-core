/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/exclusive_access/browser_view_exclusive_access_context.h"

#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_bubble_type.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_context.h"
#include "components/prefs/pref_service.h"

namespace {

// Brave lets users turn off the reminder of how to exit fullscreen.
bool ShouldHideFullscreenReminder(const ExclusiveAccessBubbleParams& params,
                                  Profile* profile) {
  return (params.type ==
              EXCLUSIVE_ACCESS_BUBBLE_TYPE_FULLSCREEN_EXIT_INSTRUCTION ||
          params.type ==
              EXCLUSIVE_ACCESS_BUBBLE_TYPE_BROWSER_FULLSCREEN_EXIT_INSTRUCTION) &&
         !profile->GetPrefs()->GetBoolean(kShowFullscreenReminder);
}

}  // namespace

#include <chrome/browser/ui/exclusive_access/browser_view_exclusive_access_context.cc>
