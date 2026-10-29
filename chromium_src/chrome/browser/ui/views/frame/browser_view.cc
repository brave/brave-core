/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/browser_view.h"

#include "base/check.h"
#include "base/check_op.h"
#include "brave/browser/ui/views/frame/layout/brave_browser_view_layout_delegate_impl.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view_delegate_impl.h"
#include "brave/browser/ui/views/infobars/brave_infobar_container_view.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_coordinator.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_bubble_type.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_context.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_layout.h"
#include "chrome/browser/ui/views/frame/multi_contents_view_delegate.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/browser/ui/web_applications/app_browser_controller.h"
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

#include <chrome/browser/ui/views/frame/browser_view.cc>

bool BrowserView::IsWebPanelContents(content::WebContents* contents) {
  NOTREACHED();
}
