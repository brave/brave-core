/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_BUTTON_CONTAINER_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_BUTTON_CONTAINER_H_

#include "base/memory/raw_ptr.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

class BraveWaybackMachineInfoBarThrobber;

// This manages button and throbber controls.
// buttons occupies all this containers area and throbber runs over the button.
// When throbbing is requested, button extends its right inset and throbber runs
// on that area.
class BraveWaybackMachineInfoBarButtonContainer : public views::View {
  METADATA_HEADER(BraveWaybackMachineInfoBarButtonContainer, views::View)
 public:
  explicit BraveWaybackMachineInfoBarButtonContainer(
      views::Button::PressedCallback callback);
  ~BraveWaybackMachineInfoBarButtonContainer() override;

  BraveWaybackMachineInfoBarButtonContainer(
      const BraveWaybackMachineInfoBarButtonContainer&) = delete;
  BraveWaybackMachineInfoBarButtonContainer& operator=(
      const BraveWaybackMachineInfoBarButtonContainer&) = delete;

  void StartThrobber();
  void StopThrobber();

  // views::View overrides:
  void Layout() override;
  gfx::Size CalculatePreferredSize() const override;

 private:
  void AdjustButtonInsets(bool add_insets);

  raw_ptr<BraveWaybackMachineInfoBarThrobber> throbber_ = nullptr;
  raw_ptr<views::View> button_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_BUTTON_CONTAINER_H_
