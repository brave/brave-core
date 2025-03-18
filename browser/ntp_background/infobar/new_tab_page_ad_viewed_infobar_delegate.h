/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NTP_BACKGROUND_INFBAR_NEW_TAB_PAGE_AD_VIEWED_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_NTP_BACKGROUND_INFBAR_NEW_TAB_PAGE_AD_VIEWED_INFOBAR_DELEGATE_H_

#include "components/infobars/core/confirm_infobar_delegate.h"

class PrefService;

namespace content {
class WebContents;
}  // namespace content

namespace ntp_background_images {

class NewTabPageAdViewedInfoBarDelegate
    : public ConfirmInfoBarDelegate {
 public:
  NewTabPageAdViewedInfoBarDelegate();
  ~NewTabPageAdViewedInfoBarDelegate() override;

  NewTabPageAdViewedInfoBarDelegate(
      const NewTabPageAdViewedInfoBarDelegate&) = delete;
  NewTabPageAdViewedInfoBarDelegate& operator=(
      const NewTabPageAdViewedInfoBarDelegate&) = delete;

  static void Create(content::WebContents* web_contents, PrefService* prefs);

 private:
  // ConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  ui::ImageModel GetIcon() const override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  std::u16string GetLinkText() const override;
  GURL GetLinkURL() const override;
  bool LinkClicked(WindowOpenDisposition disposition) override;
};

}  // namespace ntp_background_images

#endif  // BRAVE_BROWSER_NTP_BACKGROUND_INFBAR_NEW_TAB_PAGE_AD_VIEWED_INFOBAR_DELEGATE_H_
