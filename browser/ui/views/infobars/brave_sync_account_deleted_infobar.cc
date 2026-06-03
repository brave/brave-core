/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_sync_account_deleted_infobar.h"

#include <utility>

#include "base/check.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"

BraveSyncAccountDeletedInfoBar::BraveSyncAccountDeletedInfoBar(
    std::unique_ptr<BraveConfirmInfoBarDelegate> delegate)
    : BraveConfirmInfoBar(std::move(delegate)) {
  CHECK(!(GetDelegate()->GetButtons() & ConfirmInfoBarDelegate::BUTTON_CANCEL));
}

BraveSyncAccountDeletedInfoBar::~BraveSyncAccountDeletedInfoBar() = default;

void BraveSyncAccountDeletedInfoBar::Layout(PassKey) {
  // Move the link to sit just to the right of the label and push the ok_button
  // to the far right edge — producing "Text _link_   [ok_button]".
  LayoutSuperclass<BraveConfirmInfoBar>(this);

  CHECK(!cancel_button_);
  CHECK(label_);
  CHECK(link_);
  CHECK(ok_button_);

  auto* layout_provider = ChromeLayoutProvider::Get();
  const int x =
      label_->bounds().right() + layout_provider->GetDistanceMetric(
                                     views::DISTANCE_RELATED_LABEL_HORIZONTAL);

  link_->SetPosition(gfx::Point(x, OffsetY(link_)));
  ok_button_->SetPosition(
      gfx::Point(GetEndX() - ok_button_->width(), OffsetY(ok_button_)));
}
