/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_BRAVE_SYNC_ACCOUNT_DELETED_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_INFOBARS_BRAVE_SYNC_ACCOUNT_DELETED_INFOBAR_DELEGATE_H_

#include "base/compiler_specific.h"
#include "base/memory/raw_ptr.h"
#include "components/infobars/core/confirm_infobar_delegate.h"

class Browser;
class Profile;

namespace content {
class WebContents;
}

// An infobar that is run with a string, "re-create account" link and a button.
class BraveSyncAccountDeletedInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  BraveSyncAccountDeletedInfoBarDelegate(
      const BraveSyncAccountDeletedInfoBarDelegate&) = delete;
  BraveSyncAccountDeletedInfoBarDelegate& operator=(
      const BraveSyncAccountDeletedInfoBarDelegate&) = delete;

  static void Create(content::WebContents* active_web_contents,
                     Profile* profile,
                     Browser* browser);

 private:
  explicit BraveSyncAccountDeletedInfoBarDelegate(Browser* browser,
                                                  Profile* profile);
  ~BraveSyncAccountDeletedInfoBarDelegate() override;

  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  bool ShouldExpire(const NavigationDetails& details) const override;
  void InfoBarDismissed() override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  GURL GetLinkURL() const override;
  std::u16string GetLinkText() const override;
  std::u16string GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;
  bool LinkClicked(WindowOpenDisposition disposition) override;
  bool IsCloseable() const override;

  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<Browser> browser_ = nullptr;
};

#endif  // BRAVE_BROWSER_INFOBARS_BRAVE_SYNC_ACCOUNT_DELETED_INFOBAR_DELEGATE_H_
