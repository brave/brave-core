/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_INFOBAR_DELEGATE_H_

#include <memory>

#include "components/infobars/core/infobar_delegate.h"

namespace content {
class WebContents;
}  //  namespace content

class BraveWaybackMachineInfoBarDelegate : public infobars::InfoBarDelegate {
 public:
  static void Create(content::WebContents* contents);
  ~BraveWaybackMachineInfoBarDelegate() override = default;

 private:
  BraveWaybackMachineInfoBarDelegate();

  // Returns an infobar that owns |delegate|.
  static std::unique_ptr<infobars::InfoBar> CreateInfoBar(
      std::unique_ptr<BraveWaybackMachineInfoBarDelegate> delegate,
      content::WebContents* contents);

  // infobars::InfoBarDelegate overrides:
  InfoBarIdentifier GetIdentifier() const override;
  bool EqualsDelegate(
      infobars::InfoBarDelegate* delegate) const override;

  DISALLOW_COPY_AND_ASSIGN(BraveWaybackMachineInfoBarDelegate);
};

#endif  // BRAVE_BROWSER_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_INFOBAR_DELEGATE_H_
