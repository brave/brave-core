/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_sync_account_deleted_infobar.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "build/build_config.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/view.h"

BraveSyncAccountDeletedInfoBar::BraveSyncAccountDeletedInfoBar(
    std::unique_ptr<ConfirmInfoBarDelegate> delegate)
    : ConfirmInfoBar(std::move(delegate)) {
  DCHECK(
      !(GetDelegate()->GetButtons() & ConfirmInfoBarDelegate::BUTTON_CANCEL));
}

BraveSyncAccountDeletedInfoBar::~BraveSyncAccountDeletedInfoBar() {}

void BraveSyncAccountDeletedInfoBar::Layout() {
  InfoBarView::Layout();

  if (ok_button_) {
    ok_button_->SizeToPreferredSize();
  }

  int x = GetStartX();
  Views views;
  views.push_back(label_.get());
  views.push_back(link_.get());
  AssignWidths(&views, std::max(0, GetEndX() - x - NonLabelWidth()));

  ChromeLayoutProvider* layout_provider = ChromeLayoutProvider::Get();

  label_->SetPosition(gfx::Point(x, OffsetY(label_)));
  if (!label_->GetText().empty()) {
    x = label_->bounds().right() +
        layout_provider->GetDistanceMetric(
            views::DISTANCE_RELATED_LABEL_HORIZONTAL);
  }

  link_->SetPosition(gfx::Point(x, OffsetY(link_)));

  DCHECK(!cancel_button_);

  if (ok_button_) {
    ok_button_->SetPosition(
        gfx::Point(GetEndX() - ok_button_->width(), OffsetY(ok_button_)));
  }
}
