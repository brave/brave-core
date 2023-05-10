/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_REQUEST_OTR_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_INFOBARS_REQUEST_OTR_INFOBAR_DELEGATE_H_

#include "components/infobars/core/confirm_infobar_delegate.h"

namespace infobars {
class ContentInfoBarManager;
}

// This class configures an infobar shown when the user is browsing in
// Off-The-Record mode.
class RequestOTRInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  RequestOTRInfoBarDelegate();
  RequestOTRInfoBarDelegate(const RequestOTRInfoBarDelegate&) = delete;
  RequestOTRInfoBarDelegate& operator=(const RequestOTRInfoBarDelegate&) =
      delete;
  ~RequestOTRInfoBarDelegate() override;

  // Creates a request-otr infobar and delegate and adds the infobar to
  // |infobar_manager|.
  static void Create(infobars::ContentInfoBarManager* infobar_manager);

 private:
  // ConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  std::u16string GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;
};

#endif  // BRAVE_BROWSER_INFOBARS_REQUEST_OTR_INFOBAR_DELEGATE_H_
