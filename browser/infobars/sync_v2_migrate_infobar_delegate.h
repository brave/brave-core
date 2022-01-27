// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_INFOBARS_SYNC_V2_MIGRATE_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_INFOBARS_SYNC_V2_MIGRATE_INFOBAR_DELEGATE_H_

#include "base/compiler_specific.h"
#include "base/memory/raw_ptr.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "url/gurl.h"

class Browser;
class Profile;
class PrefService;

namespace brave_sync {
class Prefs;
}  // namespace brave_sync

namespace infobars {
class ContentInfoBarManager;
}  // namespace infobars

// An infobar that is run with a string, buttons, and a "Learn More" link.
class SyncV2MigrateInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  SyncV2MigrateInfoBarDelegate(const SyncV2MigrateInfoBarDelegate&) = delete;
  SyncV2MigrateInfoBarDelegate& operator=(const SyncV2MigrateInfoBarDelegate&) =
      delete;

  static void Create(infobars::ContentInfoBarManager* infobar_manager,
                     bool is_v2_user,
                     Profile* profile,
                     Browser* browser);

 private:
  explicit SyncV2MigrateInfoBarDelegate(Browser* browser, Profile* profile);
  ~SyncV2MigrateInfoBarDelegate() override;

  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  bool ShouldExpire(const NavigationDetails& details) const override;
  void InfoBarDismissed() override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  std::u16string GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;

  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<Browser> browser_ = nullptr;
};

#endif  // BRAVE_BROWSER_INFOBARS_SYNC_V2_MIGRATE_INFOBAR_DELEGATE_H_
