/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NTP_BACKGROUND_NEW_TAB_TAKEOVER_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_NTP_BACKGROUND_NEW_TAB_TAKEOVER_INFOBAR_DELEGATE_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include "components/infobars/core/infobar_delegate.h"

class PrefService;
class GURL;
enum class WindowOpenDisposition;

namespace content {
class WebContents;
}  // namespace content

namespace ui {
class ImageModel;
}  // namespace ui

namespace ntp_background_images {

class NewTabTakeoverInfoBarDelegate : public BraveConfirmInfoBarDelegate {
 public:
  explicit NewTabTakeoverInfoBarDelegate(PrefService* prefs);

  NewTabTakeoverInfoBarDelegate(const NewTabTakeoverInfoBarDelegate&) = delete;
  NewTabTakeoverInfoBarDelegate& operator=(
      const NewTabTakeoverInfoBarDelegate&) = delete;

  ~NewTabTakeoverInfoBarDelegate() override;

  static void MaybeDisplayAndIncrementCounter(
      content::WebContents* web_contents,
      PrefService* prefs);

 private:
  // ConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  ui::ImageModel GetIcon() const override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  std::u16string GetLinkText() const override;
  GURL GetLinkURL() const override;
  bool LinkClicked(WindowOpenDisposition disposition) override;
  void InfoBarDismissed() override;

  // BraveConfirmInfoBarDelegate:
  std::vector<int> GetButtonsOrder() const override;
  bool ShouldSupportMultiLine() const override;

 private:
  const raw_ptr<PrefService> prefs_;
};

}  // namespace ntp_background_images

#endif  // BRAVE_BROWSER_NTP_BACKGROUND_NEW_TAB_TAKEOVER_INFOBAR_DELEGATE_H_
