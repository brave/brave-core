/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_infobar_container_view.h"

#include <memory>

#include "base/check.h"
#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"
#include "components/infobars/core/infobar.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/view.h"
#include "ui/views/view_utils.h"

namespace {

BraveConfirmInfoBar* AsBraveConfirmInfoBar(const infobars::InfoBar* infobar,
                                           const views::View::Views& children) {
  for (views::View* child : children) {
    auto* brave_confirm_infobar =
        views::AsViewClass<BraveConfirmInfoBar>(child);
    if (brave_confirm_infobar && brave_confirm_infobar == infobar) {
      return brave_confirm_infobar;
    }
  }

  return nullptr;
}

}  // namespace

BraveInfoBarContainerView::BraveInfoBarContainerView(
    infobars::InfoBarContainer::Delegate* delegate)
    : InfoBarContainerView(delegate) {
  // To hide shadow, replace it with empty view.
  DCHECK(content_shadow_);
  RemoveChildViewT(content_shadow_.ExtractAsDangling());
  content_shadow_ = AddChildView(std::make_unique<views::View>());
}

BraveInfoBarContainerView::~BraveInfoBarContainerView() = default;

void BraveInfoBarContainerView::PlatformSpecificRemoveInfoBar(
    infobars::InfoBar* infobar) {
  // TODO(https://github.com/brave/brave-browser/issues/61574): Add
  // queued-count pill support for `BraveConfirmInfoBar`.
  BraveConfirmInfoBar* const brave_confirm_infobar =
      AsBraveConfirmInfoBar(infobar, children());
  if (brave_confirm_infobar) {
    return RemoveChildView(brave_confirm_infobar);
  }

  InfoBarContainerView::PlatformSpecificRemoveInfoBar(infobar);
}

BEGIN_METADATA(BraveInfoBarContainerView)
END_METADATA
