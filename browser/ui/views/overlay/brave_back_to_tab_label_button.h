/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_OVERLAY_BRAVE_BACK_TO_TAB_LABEL_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_OVERLAY_BRAVE_BACK_TO_TAB_LABEL_BUTTON_H_

#include "chrome/browser/ui/views/overlay/back_to_tab_label_button.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveBackToTabLabelButton : public BackToTabLabelButton {
  METADATA_HEADER(BraveBackToTabLabelButton, BackToTabLabelButton)

 public:
  explicit BraveBackToTabLabelButton(PressedCallback callback);
  ~BraveBackToTabLabelButton() override;

  // BackToTabLabelButton:
  void OnThemeChanged() override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_OVERLAY_BRAVE_BACK_TO_TAB_LABEL_BUTTON_H_
