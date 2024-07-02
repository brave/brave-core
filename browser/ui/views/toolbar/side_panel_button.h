/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SIDE_PANEL_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SIDE_PANEL_BUTTON_H_

#include "base/memory/raw_ref.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "components/prefs/pref_member.h"
#include "ui/base/metadata/metadata_header_macros.h"

class Browser;

class SidePanelButton : public ToolbarButton {
  METADATA_HEADER(SidePanelButton, ToolbarButton)

 public:
  explicit SidePanelButton(Browser* browser);
  SidePanelButton(const SidePanelButton&) = delete;
  SidePanelButton& operator=(const SidePanelButton&) = delete;
  ~SidePanelButton() override;

 private:
  void ButtonPressed();

  // Updates the vector icon used when the PrefChangeRegistrar listens to a
  // change. When the side panel should open to the right side of the browser
  // the default vector icon is used. When the side panel should open to the
  // left side of the browser the flipped vector icon is used.
  void UpdateToolbarButtonIcon();

  const raw_ref<Browser> browser_;

  // Observes and listens to side panel alignment changes.
  BooleanPrefMember sidebar_alignment_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_SIDE_PANEL_BUTTON_H_
