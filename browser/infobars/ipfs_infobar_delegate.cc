/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/ipfs_infobar_delegate.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/ui/brave_pages.h"
#include "brave/components/ipfs/browser/brave_ipfs_client_updater.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/infobars/core/infobar.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/vector_icons.h"


// static
void IPFSInfoBarDelegate::Create(InfoBarService* infobar_service) {
  infobar_service->AddInfoBar(infobar_service->CreateConfirmInfoBar(
      std::unique_ptr<ConfirmInfoBarDelegate>(
          new IPFSInfoBarDelegate())));
}

IPFSInfoBarDelegate::IPFSInfoBarDelegate() {
}

IPFSInfoBarDelegate::~IPFSInfoBarDelegate() {}

infobars::InfoBarDelegate::InfoBarIdentifier
IPFSInfoBarDelegate::GetIdentifier() const {
  return IPFS_INFOBAR_DELEGATE;
}

const gfx::VectorIcon& IPFSInfoBarDelegate::GetVectorIcon() const {
  return views::kInfoIcon;
}

void IPFSInfoBarDelegate::InfoBarDismissed() {
}

base::string16 IPFSInfoBarDelegate::GetMessageText() const {
  return l10n_util::GetStringUTF16(IDS_BRAVE_IPFS_INSTALL);
}

int IPFSInfoBarDelegate::GetButtons() const {
  return BUTTON_OK | BUTTON_CANCEL;
}

base::string16 IPFSInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  if (button == BUTTON_CANCEL) {
    return l10n_util::GetStringUTF16(IDS_BRAVE_IPFS_SETTINGS);
  }

  return l10n_util::GetStringUTF16(IDS_BRAVE_IPFS_ENABLE_IPFS);
}

base::string16 IPFSInfoBarDelegate::GetLinkText() const {
  return base::string16();
}

GURL IPFSInfoBarDelegate::GetLinkURL() const {
  return GURL();  // No learn more link for now.
}

bool IPFSInfoBarDelegate::Accept() {
  g_brave_browser_process->ipfs_client_updater()->Register();
  return true;
}

bool IPFSInfoBarDelegate::Cancel() {
  content::WebContents* web_contents =
    InfoBarService::WebContentsFromInfoBar(infobar());
  if (web_contents) {
    Browser* browser = chrome::FindBrowserWithWebContents(web_contents);
    brave::ShowExtensionSettings(browser);
  }

  return true;
}
