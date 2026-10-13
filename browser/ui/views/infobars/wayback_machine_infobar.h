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

 private:
  WaybackMachineInfoBarDelegate* GetDelegate();
  void OnCountdownChanged();
  void OnRedirectClicked();
  void OnCancelClicked();
  void OnLearnMoreClicked();

  raw_ptr<views::MdTextButton> redirect_button_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_WAYBACK_MACHINE_INFOBAR_H_
