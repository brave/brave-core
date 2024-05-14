/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_frame_mac.h"

#include "base/feature_list.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "components/remote_cocoa/common/native_widget_ns_window_host.mojom.h"

BraveBrowserFrameMac::BraveBrowserFrameMac(BrowserFrame* browser_frame,
                                           BrowserView* browser_view)
    : BrowserFrameMac(browser_frame, browser_view),
      browser_(browser_view->browser()),
      browser_view_(browser_view) {}

BraveBrowserFrameMac::~BraveBrowserFrameMac() = default;

void BraveBrowserFrameMac::GetWindowFrameTitlebarHeight(
    bool* override_titlebar_height,
    float* titlebar_height) {
  if (tabs::utils::ShouldShowVerticalTabs(browser_)) {
    if (!tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser_)) {
      // In this case, titlbar height should be the same as toolbar height.
      *titlebar_height = browser_view_->toolbar()->GetPreferredSize().height();
      *override_titlebar_height = true;
      return;
    }

    // Otherwise, don't override titlebar height. The titlbar will be aligned
    // to the center of the given height automatically.
    return;
  }

  BrowserFrameMac::GetWindowFrameTitlebarHeight(override_titlebar_height,
                                                titlebar_height);
}

void BraveBrowserFrameMac::ValidateUserInterfaceItem(
    int32_t tag,
    remote_cocoa::mojom::ValidateUserInterfaceItemResult* result) {
  BrowserFrameMac::ValidateUserInterfaceItem(tag, result);

  if (tag != IDC_TOGGLE_TAB_MUTE || !result->enable) {
    return;
  }

  // Update toggle state for tab mute menu bar entry.
  Browser* browser = browser_view_->browser();
  TabStripModel* model = browser->tab_strip_model();
  result->set_toggle_state = true;
  result->new_toggle_state = !model->empty() && model->GetActiveWebContents() &&
                             model->GetActiveWebContents()->IsAudioMuted();
}

bool BraveBrowserFrameMac::ExecuteCommand(
    int32_t command,
    WindowOpenDisposition window_open_disposition,
    bool is_before_first_responder) {
  if (base::FeatureList::IsEnabled(tabs::features::kBraveSharedPinnedTabs)) {
    // is_before_first_responder tells whether or not the app/window was in
    // focus while the keyboard command was fired. In current method, it helps
    // in distinguishing the 'file -> close tab' (false, as toolbar was in
    // focus) command from 'ctrl + w' (true, as tab was in focus) command.
    Browser* browser = browser_view_->browser();
    int active_tab_index = browser->tab_strip_model()->active_index();
    if (command == IDC_CLOSE_TAB && is_before_first_responder &&
        browser->tab_strip_model()->IsTabPinned(active_tab_index)) {
      // Ignoring the ctrl+w command when the active tab is shared pinned
      return true;
    }
  }

  return BrowserFrameMac::ExecuteCommand(command, window_open_disposition,
                                         is_before_first_responder);
}
