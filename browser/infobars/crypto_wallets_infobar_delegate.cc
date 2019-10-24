/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/crypto_wallets_infobar_delegate.h"

#include <memory>
#include <utility>

#include "brave/browser/ui/brave_pages.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "brave/common/url_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/grit/chromium_strings.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/vector_icons.h"

// static
void CryptoWalletsInfoBarDelegate::Create(InfoBarService* infobar_service,
    bool metamask_installed) {
  infobar_service->AddInfoBar(infobar_service->CreateConfirmInfoBar(
      std::unique_ptr<ConfirmInfoBarDelegate>(
          new CryptoWalletsInfoBarDelegate(metamask_installed))));
}

CryptoWalletsInfoBarDelegate::CryptoWalletsInfoBarDelegate(
    bool metamask_installed) : metamask_installed_(metamask_installed) {
}

CryptoWalletsInfoBarDelegate::~CryptoWalletsInfoBarDelegate() {}

infobars::InfoBarDelegate::InfoBarIdentifier
CryptoWalletsInfoBarDelegate::GetIdentifier() const {
  return (InfoBarIdentifier)CRYPTO_WALLETS_INFOBAR_DELEGATE;
}

const gfx::VectorIcon& CryptoWalletsInfoBarDelegate::GetVectorIcon() const {
  return views::kInfoIcon;
}

void CryptoWalletsInfoBarDelegate::InfoBarDismissed() {
}

base::string16 CryptoWalletsInfoBarDelegate::GetMessageText() const {
  if (metamask_installed_) {
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_CRYPTO_WALLETS_METAMASK_INFOBAR_TEXT);
  }
  return l10n_util::GetStringUTF16(IDS_BRAVE_CRYPTO_WALLETS_INFOBAR_TEXT);
}

int CryptoWalletsInfoBarDelegate::GetButtons() const {
  if (metamask_installed_) {
    return BUTTON_OK | BUTTON_CANCEL;
  }
  return BUTTON_OK;
}

base::string16 CryptoWalletsInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  if (button == BUTTON_CANCEL) {
    return l10n_util::GetStringUTF16(IDS_BRAVE_CRYPTO_WALLETS_USE_METAMASK);
  }

  return metamask_installed_ ?
      l10n_util::GetStringUTF16(IDS_BRAVE_CRYPTO_WALLETS_USE_CRYPTO_WALLETS) :
      l10n_util::GetStringUTF16(IDS_BRAVE_CRYPTO_WALLETS_SETUP);
}

base::string16 CryptoWalletsInfoBarDelegate::GetLinkText() const {
  return base::string16();
}

GURL CryptoWalletsInfoBarDelegate::GetLinkURL() const {
  return GURL();  // No learn more link for now.
}

bool CryptoWalletsInfoBarDelegate::Accept() {
  if (infobar() && infobar()->owner()) {
    content::WebContents* web_contents =
      InfoBarService::WebContentsFromInfoBar(infobar());
    if (web_contents) {
      Browser* browser = chrome::FindBrowserWithWebContents(web_contents);
      brave::ShowBraveWallet(browser);
    }
  }
  return true;
}

bool CryptoWalletsInfoBarDelegate::Cancel() {
  content::WebContents* web_contents =
      InfoBarService::WebContentsFromInfoBar(infobar());
  if (web_contents) {
    user_prefs::UserPrefs::Get(web_contents->GetBrowserContext())->
        SetBoolean(kBraveWalletEnabled, false);
  }
  return true;
}
