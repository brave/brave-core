/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_REQUEST_OTR_INFOBAR_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_REQUEST_OTR_INFOBAR_H_

#include <memory>

#include "chrome/browser/ui/views/infobars/confirm_infobar.h"

// The customized ConfirmInfoBar:
// "Text _link_                     [ok_button]"
// cancel_button is not supported

class RequestOTRInfoBar : public ConfirmInfoBar {
 public:
  explicit RequestOTRInfoBar(std::unique_ptr<ConfirmInfoBarDelegate> delegate);

  RequestOTRInfoBar(const RequestOTRInfoBar&) = delete;
  RequestOTRInfoBar& operator=(const RequestOTRInfoBar&) = delete;

  ~RequestOTRInfoBar() override;

  // InfoBarView:
  // void Layout() override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_REQUEST_OTR_INFOBAR_H_
