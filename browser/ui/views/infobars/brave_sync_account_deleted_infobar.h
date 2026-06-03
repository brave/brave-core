/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_SYNC_ACCOUNT_DELETED_INFOBAR_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_SYNC_ACCOUNT_DELETED_INFOBAR_H_

#include <memory>

#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"

// Customized BraveConfirmInfoBar:
//   "Text _link_                     [ok_button]"
// cancel_button is not supported.
class BraveSyncAccountDeletedInfoBar : public BraveConfirmInfoBar {
 public:
  explicit BraveSyncAccountDeletedInfoBar(
      std::unique_ptr<BraveConfirmInfoBarDelegate> delegate);

  BraveSyncAccountDeletedInfoBar(const BraveSyncAccountDeletedInfoBar&) =
      delete;
  BraveSyncAccountDeletedInfoBar& operator=(
      const BraveSyncAccountDeletedInfoBar&) = delete;

  ~BraveSyncAccountDeletedInfoBar() override;

  // BraveConfirmInfoBar:
  void Layout(PassKey) override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_SYNC_ACCOUNT_DELETED_INFOBAR_H_
