/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_INFOBAR_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_INFOBAR_DELEGATE_H_

#include <memory>

#include "components/infobars/core/infobar_delegate.h"

class BraveWaybackMachineInfoBarDelegate : public infobars::InfoBarDelegate {
 public:
  BraveWaybackMachineInfoBarDelegate();
  ~BraveWaybackMachineInfoBarDelegate() override;

  BraveWaybackMachineInfoBarDelegate(
      const BraveWaybackMachineInfoBarDelegate&) = delete;
  BraveWaybackMachineInfoBarDelegate& operator=(
      const BraveWaybackMachineInfoBarDelegate&) = delete;

 private:
  // infobars::InfoBarDelegate overrides:
  InfoBarIdentifier GetIdentifier() const override;
  bool EqualsDelegate(
      infobars::InfoBarDelegate* delegate) const override;
};

#endif  // BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_INFOBAR_DELEGATE_H_
