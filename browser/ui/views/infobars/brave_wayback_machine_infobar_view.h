/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_VIEW_H_

#include <memory>

#include "chrome/browser/ui/views/infobars/infobar_view.h"

namespace content {
class WebContents;
}

class BraveWaybackMachineInfoBarDelegate;

class BraveWaybackMachineInfoBarView : public InfoBarView {
 public:
  BraveWaybackMachineInfoBarView(
      std::unique_ptr<BraveWaybackMachineInfoBarDelegate> delegate,
      content::WebContents* contents);
  ~BraveWaybackMachineInfoBarView() override;

  BraveWaybackMachineInfoBarView(
      const BraveWaybackMachineInfoBarView&) = delete;
  BraveWaybackMachineInfoBarView& operator=(
      const BraveWaybackMachineInfoBarView&) = delete;

 private:
  // InfoBarView overrides:
  void Layout() override;

  views::View* sub_views_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_VIEW_H_
