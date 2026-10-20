// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_TRANSFER_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_TRANSFER_BUTTON_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BrowserWindowInterface;

// Opens brave://transfer.
class BraveTransferButton : public ToolbarButton {
  METADATA_HEADER(BraveTransferButton, ToolbarButton)
 public:
  explicit BraveTransferButton(
      BrowserWindowInterface* browser_window_interface);
  BraveTransferButton(const BraveTransferButton&) = delete;
  BraveTransferButton& operator=(const BraveTransferButton&) = delete;
  ~BraveTransferButton() override;

 private:
  void ButtonPressed();

  raw_ptr<BrowserWindowInterface> browser_window_interface_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_BRAVE_TRANSFER_BUTTON_H_
