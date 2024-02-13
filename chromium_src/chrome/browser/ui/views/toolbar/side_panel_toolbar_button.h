// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_SIDE_PANEL_TOOLBAR_BUTTON_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_SIDE_PANEL_TOOLBAR_BUTTON_H_

#include "components/prefs/pref_member.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/models/simple_menu_model.h"

#define SidePanelToolbarButton SidePanelToolbarButton_ChromiumImpl

#include "src/chrome/browser/ui/views/toolbar/side_panel_toolbar_button.h"  // IWYU pragma: export

#undef SidePanelToolbarButton

class SidePanelToolbarButton : public SidePanelToolbarButton_ChromiumImpl {
  METADATA_HEADER(SidePanelToolbarButton, SidePanelToolbarButton_ChromiumImpl)
 public:
  explicit SidePanelToolbarButton(Browser* browser);

  SidePanelToolbarButton(const SidePanelToolbarButton&) = delete;
  SidePanelToolbarButton& operator=(const SidePanelToolbarButton&) = delete;
  ~SidePanelToolbarButton() override;

 private:
  void UpdateButtonImage();

  BooleanPrefMember sidebar_alignment_;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_SIDE_PANEL_TOOLBAR_BUTTON_H_
