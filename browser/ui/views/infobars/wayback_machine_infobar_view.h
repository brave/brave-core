// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_WAYBACK_MACHINE_INFOBAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_WAYBACK_MACHINE_INFOBAR_VIEW_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/views/infobars/infobar_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class WaybackMachineInfoBarDelegate;

namespace ui {
class Event;
}

namespace views {
class MdTextButton;
}

// Infobar shown before automatically loading an archived version of a missing
// page. Shows a title, a message with a "Learn more" link, a cancel button and
// a button that loads the archived version and shows the countdown.
class WaybackMachineInfoBarView : public InfoBarView {
  METADATA_HEADER(WaybackMachineInfoBarView, InfoBarView)

 public:
  explicit WaybackMachineInfoBarView(
      std::unique_ptr<WaybackMachineInfoBarDelegate> delegate);
  WaybackMachineInfoBarView(const WaybackMachineInfoBarView&) = delete;
  WaybackMachineInfoBarView& operator=(const WaybackMachineInfoBarView&) =
      delete;
  ~WaybackMachineInfoBarView() override;

  views::MdTextButton* cancel_button_for_testing() { return cancel_button_; }
  views::MdTextButton* redirect_button_for_testing() {
    return redirect_button_;
  }

 private:
  WaybackMachineInfoBarDelegate* GetDelegate();

  void UpdateRedirectButtonText();
  void OnCancelButtonPressed();
  void OnRedirectButtonPressed();
  void OnLearnMoreClicked(const ui::Event& event);

  raw_ptr<views::MdTextButton> cancel_button_ = nullptr;
  raw_ptr<views::MdTextButton> redirect_button_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_WAYBACK_MACHINE_INFOBAR_VIEW_H_
