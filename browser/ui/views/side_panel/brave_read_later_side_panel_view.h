/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_READ_LATER_SIDE_PANEL_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_READ_LATER_SIDE_PANEL_VIEW_H_

#include "base/functional/callback_forward.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_view_base.h"
#include "ui/base/metadata/metadata_header_macros.h"

class Browser;
class SidePanelEntryScope;

// Gives reading list specific header view with web view.
class BraveReadLaterSidePanelView : public BraveSidePanelViewBase {
  METADATA_HEADER(BraveReadLaterSidePanelView, BraveSidePanelViewBase)

 public:
  BraveReadLaterSidePanelView(Browser* browser,
                              SidePanelEntryScope& scope,
                              base::RepeatingClosure close_cb);
  ~BraveReadLaterSidePanelView() override;
  BraveReadLaterSidePanelView(const BraveReadLaterSidePanelView&) = delete;
  BraveReadLaterSidePanelView& operator=(const BraveReadLaterSidePanelView&) =
      delete;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_READ_LATER_SIDE_PANEL_VIEW_H_
