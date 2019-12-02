/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wayback_machine/brave_wayback_machine_infobar_delegate.h"

#include "base/memory/ptr_util.h"
#include "brave/browser/infobars/brave_infobar_delegate.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "components/infobars/core/infobar.h"

using InfoBarIdentifier = infobars::InfoBarDelegate::InfoBarIdentifier;

// static
void BraveWaybackMachineInfoBarDelegate::Create(
    content::WebContents* contents) {
  InfoBarService* infobar_service = InfoBarService::FromWebContents(contents);
  infobar_service->AddInfoBar(
      BraveWaybackMachineInfoBarDelegate::CreateInfoBar(
          base::WrapUnique(new BraveWaybackMachineInfoBarDelegate),
          contents),
      true);
}

BraveWaybackMachineInfoBarDelegate::BraveWaybackMachineInfoBarDelegate() {
}

InfoBarIdentifier BraveWaybackMachineInfoBarDelegate::GetIdentifier() const {
  return static_cast<InfoBarIdentifier>(WAYBACK_MACHINE_INFOBAR_DELEGATE);
}

bool BraveWaybackMachineInfoBarDelegate::EqualsDelegate(
    infobars::InfoBarDelegate* delegate) const {
  return delegate->GetIdentifier() == GetIdentifier();
}
