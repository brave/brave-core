/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_CRYPTO_WALLETS_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_INFOBARS_CRYPTO_WALLETS_INFOBAR_DELEGATE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "url/gurl.h"

#include "brave/browser/infobars/brave_infobar_delegate.h"

class InfoBarService;
class PrefService;

// An infobar that is run with a string, buttons, and a "Learn More" link.
class CryptoWalletsInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  static void Create(InfoBarService* infobar_service, bool metamask_installed);

 private:
  explicit CryptoWalletsInfoBarDelegate(bool metamask_installed);
  ~CryptoWalletsInfoBarDelegate() override;

  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  void InfoBarDismissed() override;
  base::string16 GetMessageText() const override;
  int GetButtons() const override;
  base::string16 GetButtonLabel(InfoBarButton button) const override;
  base::string16 GetLinkText() const override;
  GURL GetLinkURL() const override;
  bool Accept() override;
  bool Cancel() override;

  bool metamask_installed_;

  DISALLOW_COPY_AND_ASSIGN(CryptoWalletsInfoBarDelegate);
};

#endif  // BRAVE_BROWSER_INFOBARS_CRYPTO_WALLETS_INFOBAR_DELEGATE_H_
