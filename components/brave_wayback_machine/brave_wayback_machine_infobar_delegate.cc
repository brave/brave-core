/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wayback_machine/brave_wayback_machine_infobar_delegate.h"

using InfoBarIdentifier = infobars::InfoBarDelegate::InfoBarIdentifier;

BraveWaybackMachineInfoBarDelegate::
BraveWaybackMachineInfoBarDelegate() = default;

BraveWaybackMachineInfoBarDelegate::
~BraveWaybackMachineInfoBarDelegate() = default;

InfoBarIdentifier BraveWaybackMachineInfoBarDelegate::GetIdentifier() const {
  return WAYBACK_MACHINE_INFOBAR_DELEGATE;
}

bool BraveWaybackMachineInfoBarDelegate::EqualsDelegate(
    infobars::InfoBarDelegate* delegate) const {
  return delegate->GetIdentifier() == GetIdentifier();
}
