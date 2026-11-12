// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_WAYBACK_MACHINE_INFOBAR_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_WAYBACK_MACHINE_INFOBAR_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/views/infobars/infobar_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class WaybackMachineInfoBarDelegate;

namespace views {
class Label;
class MdTextButton;
}  // namespace views

// Infobar shown when an automatic check finds a snapshot of a missing page.
// Updates its countdown button as the delegate counts down.
class WaybackMachineInfoBar : public InfoBarView {
  METADATA_HEADER(WaybackMachineInfoBar, InfoBarView)

 public:
  explicit WaybackMachineInfoBar(
      std::unique_ptr<WaybackMachineInfoBarDelegate> delegate);
  WaybackMachineInfoBar(const WaybackMachineInfoBar&) = delete;
  WaybackMachineInfoBar& operator=(const WaybackMachineInfoBar&) = delete;
  ~WaybackMachineInfoBar() override;

  views::Label* description_label_for_testing() { return description_label_; }
  views::MdTextButton* cancel_button_for_testing() { return cancel_button_; }
  views::MdTextButton* redirect_button_for_testing() {
    return redirect_button_;
  }

 private:
  WaybackMachineInfoBarDelegate* GetDelegate();
  void OnCountdownChanged();
  void OnRedirectClicked();
  void OnCancelClicked();

  raw_ptr<views::Label> description_label_ = nullptr;
  raw_ptr<views::MdTextButton> cancel_button_ = nullptr;
  raw_ptr<views::MdTextButton> redirect_button_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_WAYBACK_MACHINE_INFOBAR_H_
