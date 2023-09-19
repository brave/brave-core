/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_BOOKMARKS_SIDE_PANEL_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_BOOKMARKS_SIDE_PANEL_VIEW_H_

#include "base/functional/callback_forward.h"
#include "base/scoped_observation.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"

class Browser;

// Gives bookmarks panel specific header view with web view.
class BraveBookmarksSidePanelView : public views::View,
                                    public views::ViewObserver {
 public:
  explicit BraveBookmarksSidePanelView(Browser* browser);
  ~BraveBookmarksSidePanelView() override;
  BraveBookmarksSidePanelView(const BraveBookmarksSidePanelView&) = delete;
  BraveBookmarksSidePanelView& operator=(const BraveBookmarksSidePanelView&) =
      delete;

 private:
  void OnViewVisibilityChanged(views::View* observed_view,
                               views::View* starting_view) override;

  base::ScopedObservation<views::View, views::ViewObserver> observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_BOOKMARKS_SIDE_PANEL_VIEW_H_
