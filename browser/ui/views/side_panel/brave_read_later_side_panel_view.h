/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_READ_LATER_SIDE_PANEL_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_READ_LATER_SIDE_PANEL_VIEW_H_

#include "base/functional/callback_forward.h"
#include "base/scoped_observation.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"

class Browser;

// Gives reading list specific header view with web view.
class BraveReadLaterSidePanelView : public views::View,
                                    public views::ViewObserver {
 public:
  BraveReadLaterSidePanelView(Browser* browser,
                              base::RepeatingClosure close_cb);
  ~BraveReadLaterSidePanelView() override;
  BraveReadLaterSidePanelView(const BraveReadLaterSidePanelView&) = delete;
  BraveReadLaterSidePanelView& operator=(const BraveReadLaterSidePanelView&) =
      delete;

 private:
  void OnViewVisibilityChanged(views::View* observed_view,
                               views::View* starting_view) override;

  base::ScopedObservation<views::View, views::ViewObserver> observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_READ_LATER_SIDE_PANEL_VIEW_H_
