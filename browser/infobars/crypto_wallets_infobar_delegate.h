/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_CRYPTO_WALLETS_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_INFOBARS_CRYPTO_WALLETS_INFOBAR_DELEGATE_H_

#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "url/gurl.h"

class InfoBarService;
class PrefService;

namespace content {
class WebContents;
}

// An infobar that is run with a string, buttons, and a "Learn More" link.
class CryptoWalletsInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  enum class InfobarSubType { LOAD_CRYPTO_WALLETS, GENERIC_SETUP };
  static void Create(InfoBarService* infobar_service, InfobarSubType subtype);

 private:
  explicit CryptoWalletsInfoBarDelegate(InfobarSubType subtype);
  CryptoWalletsInfoBarDelegate(const CryptoWalletsInfoBarDelegate&) = delete;
  CryptoWalletsInfoBarDelegate& operator=(const CryptoWalletsInfoBarDelegate&) =
      delete;
  ~CryptoWalletsInfoBarDelegate() override;

  void OnCryptoWalletsLoaded(content::WebContents*);
  bool ShouldShowLazyLoadInfobar();
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

  InfobarSubType subtype_;

  base::WeakPtrFactory<CryptoWalletsInfoBarDelegate> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_INFOBARS_CRYPTO_WALLETS_INFOBAR_DELEGATE_H_
