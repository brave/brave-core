/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_SYNC_CANNOT_RUN_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_INFOBARS_SYNC_CANNOT_RUN_INFOBAR_DELEGATE_H_

#include "components/infobars/core/confirm_infobar_delegate.h"

class Browser;
class Profile;

namespace brave_sync {
class Prefs;
}  // namespace brave_sync

namespace infobars {
class ContentInfoBarManager;
}  // namespace infobars

class SyncCannotRunInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  SyncCannotRunInfoBarDelegate(const SyncCannotRunInfoBarDelegate&) = delete;
  SyncCannotRunInfoBarDelegate& operator=(const SyncCannotRunInfoBarDelegate&) =
      delete;

  static void Create(infobars::ContentInfoBarManager* infobar_manager,
                     Profile* profile,
                     Browser* browser);

 private:
  explicit SyncCannotRunInfoBarDelegate(Browser* browser, Profile* profile);
  ~SyncCannotRunInfoBarDelegate() override;

  // ConfirmInfoBarDelegate overrides
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  bool ShouldExpire(const NavigationDetails& details) const override;
  void InfoBarDismissed() override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  std::u16string GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;
  bool Cancel() override;

  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<Browser> browser_ = nullptr;
};

#endif  // BRAVE_BROWSER_INFOBARS_SYNC_CANNOT_RUN_INFOBAR_DELEGATE_H_
