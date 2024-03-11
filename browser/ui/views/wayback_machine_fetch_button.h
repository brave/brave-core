/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WAYBACK_MACHINE_FETCH_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_WAYBACK_MACHINE_FETCH_BUTTON_H_

#include "base/memory/raw_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

class WaybackMachineThrobber;

// This manages button and throbber controls.
// buttons occupies all this containers area and throbber runs over the button.
// When throbbing is requested, button extends its right inset and throbber runs
// on that area.
class WaybackMachineFetchButton : public views::View {
  METADATA_HEADER(WaybackMachineFetchButton, views::View)
 public:
  explicit WaybackMachineFetchButton(views::Button::PressedCallback callback);
  ~WaybackMachineFetchButton() override;

  WaybackMachineFetchButton(const WaybackMachineFetchButton&) = delete;
  WaybackMachineFetchButton& operator=(const WaybackMachineFetchButton&) =
      delete;

  void StartThrobber();
  void StopThrobber();

  // views::View overrides:
  void Layout(PassKey) override;
  gfx::Size CalculatePreferredSize() const override;

 private:
  void AdjustButtonInsets(bool add_insets);

  raw_ptr<WaybackMachineThrobber> throbber_ = nullptr;
  raw_ptr<views::View> button_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WAYBACK_MACHINE_FETCH_BUTTON_H_
