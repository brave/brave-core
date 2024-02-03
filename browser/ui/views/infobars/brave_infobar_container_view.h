/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_INFOBAR_CONTAINER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_INFOBAR_CONTAINER_VIEW_H_

#include "chrome/browser/ui/views/infobars/infobar_container_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveInfoBarContainerView : public InfoBarContainerView {
  METADATA_HEADER(BraveInfoBarContainerView, InfoBarContainerView)
 public:
  explicit BraveInfoBarContainerView(
      infobars::InfoBarContainer::Delegate* delegate);
  BraveInfoBarContainerView(const BraveInfoBarContainerView&) = delete;
  BraveInfoBarContainerView& operator=(const BraveInfoBarContainerView&) =
      delete;
  ~BraveInfoBarContainerView() override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_INFOBAR_CONTAINER_VIEW_H_
