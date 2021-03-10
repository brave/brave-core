/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/crypto_wallets_infobar_delegate.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/ui/brave_pages.h"
#include "brave/common/url_constants.h"
#include "brave/components/brave_wallet/brave_wallet_constants.h"
#include "brave/components/brave_wallet/brave_wallet_service.h"
#include "brave/components/brave_wallet/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/browser/profiles/profile.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "extensions/common/constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/vector_icons.h"

// static
void CryptoWalletsInfoBarDelegate::Create(InfoBarService* infobar_service,
    CryptoWalletsInfoBarDelegate::InfobarSubType subtype) {
  infobar_service->AddInfoBar(infobar_service->CreateConfirmInfoBar(
      std::unique_ptr<ConfirmInfoBarDelegate>(
          new CryptoWalletsInfoBarDelegate(subtype))));
}

CryptoWalletsInfoBarDelegate::CryptoWalletsInfoBarDelegate(
    CryptoWalletsInfoBarDelegate::InfobarSubType subtype) :
        subtype_(subtype) {
}

CryptoWalletsInfoBarDelegate::~CryptoWalletsInfoBarDelegate() {}

infobars::InfoBarDelegate::InfoBarIdentifier
CryptoWalletsInfoBarDelegate::GetIdentifier() const {
  return CRYPTO_WALLETS_INFOBAR_DELEGATE;
}

const gfx::VectorIcon& CryptoWalletsInfoBarDelegate::GetVectorIcon() const {
  return views::kInfoIcon;
}

void CryptoWalletsInfoBarDelegate::InfoBarDismissed() {
}

base::string16 CryptoWalletsInfoBarDelegate::GetMessageText() const {
  if (subtype_ == InfobarSubType::LOAD_CRYPTO_WALLETS) {
    return l10n_util::GetStringUTF16(IDS_BRAVE_CRYPTO_WALLETS_LAZY_LOAD_TEXT);
  }
  return l10n_util::GetStringUTF16(IDS_BRAVE_CRYPTO_WALLETS_INFOBAR_TEXT);
}

int CryptoWalletsInfoBarDelegate::GetButtons() const {
  return BUTTON_OK | BUTTON_CANCEL;
}

base::string16 CryptoWalletsInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  if (subtype_ == InfobarSubType::LOAD_CRYPTO_WALLETS) {
    if (button == BUTTON_CANCEL) {
      return l10n_util::GetStringUTF16(IDS_BRAVE_CRYPTO_WALLETS_SETTINGS);
    }
    return l10n_util::GetStringUTF16(
        IDS_BRAVE_CRYPTO_WALLETS_START_AND_RELOAD);
  }

  if (button == BUTTON_CANCEL) {
    return l10n_util::GetStringUTF16(IDS_BRAVE_CRYPTO_WALLETS_DONT_ASK);
  }

  return l10n_util::GetStringUTF16(
      IDS_BRAVE_CRYPTO_WALLETS_SETUP_CRYPTO_WALLETS);
}

base::string16 CryptoWalletsInfoBarDelegate::GetLinkText() const {
  return l10n_util::GetStringUTF16(IDS_LEARN_MORE);
}

GURL CryptoWalletsInfoBarDelegate::GetLinkURL() const {
  return GURL(kCryptoWalletsLearnMoreURL);
}

bool CryptoWalletsInfoBarDelegate::Accept() {
  if (subtype_ == InfobarSubType::LOAD_CRYPTO_WALLETS) {
    content::WebContents* web_contents =
      InfoBarService::WebContentsFromInfoBar(infobar());
    if (web_contents) {
      auto* browser_context = web_contents->GetBrowserContext();
      Profile* profile = Profile::FromBrowserContext(browser_context);
      auto* service = BraveWalletServiceFactory::GetForProfile(profile);
      service->MaybeLoadCryptoWalletsExtension(
          base::BindOnce(&CryptoWalletsInfoBarDelegate::OnCryptoWalletsLoaded,
                         base::Unretained(this), web_contents));
    }
    return true;
  }
  if (infobar() && infobar()->owner()) {
    content::WebContents* web_contents =
      InfoBarService::WebContentsFromInfoBar(infobar());
    if (web_contents) {
      auto* browser_context = web_contents->GetBrowserContext();
      user_prefs::UserPrefs::Get(browser_context)->
          SetInteger(kBraveWalletWeb3Provider,
              static_cast<int>(BraveWalletWeb3ProviderTypes::CRYPTO_WALLETS));
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
    if (subtype_ == InfobarSubType::GENERIC_SETUP) {
      auto* browser_context = web_contents->GetBrowserContext();
      user_prefs::UserPrefs::Get(browser_context)->
          SetInteger(kBraveWalletWeb3Provider,
              static_cast<int>(BraveWalletWeb3ProviderTypes::NONE));
      return true;
    }
    Browser* browser = chrome::FindBrowserWithWebContents(web_contents);
    brave::ShowExtensionSettings(browser);
  }
  return true;
}

void CryptoWalletsInfoBarDelegate::OnCryptoWalletsLoaded(
    content::WebContents* web_contents) {
  web_contents->GetController().Reload(content::ReloadType::NORMAL, true);
}
