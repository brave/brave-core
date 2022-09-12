// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_SIDE_PANEL_TOOLBAR_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_SIDE_PANEL_TOOLBAR_BUTTON_H_

#include <memory>

#include "chrome/browser/ui/views/toolbar/side_panel_toolbar_button.h"
#include "ui/base/models/simple_menu_model.h"

class BraveSidePanelToolbarButton : public SidePanelToolbarButton {
 public:
  explicit BraveSidePanelToolbarButton(Browser* browser);

  BraveSidePanelToolbarButton(const BraveSidePanelToolbarButton&) = delete;
  BraveSidePanelToolbarButton& operator=(const BraveSidePanelToolbarButton&) =
      delete;
  ~BraveSidePanelToolbarButton() override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_SIDE_PANEL_TOOLBAR_BUTTON_H_
