/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/brave_shields_reload_infobar_delegate.h"

#include <memory>

#include "chrome/browser/infobars/confirm_infobar_creator.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/vector_icons.h"

// static
void BraveShieldsReloadInfoBarDelegate::Create(
    infobars::ContentInfoBarManager* infobar_manager) {
  if (!infobar_manager) {
    return;
  }

  // Prevent duplicate reload infobars.
  for (size_t i = 0; i < infobar_manager->infobars().size(); ++i) {
    infobars::InfoBar* infobar = infobar_manager->infobars()[i];
    if (infobar->delegate()->GetIdentifier() ==
        BRAVE_SHIELDS_RELOAD_INFOBAR_DELEGATE) {
      return;
    }
  }

  infobar_manager->AddInfoBar(
      CreateConfirmInfoBar(std::unique_ptr<ConfirmInfoBarDelegate>(
          new BraveShieldsReloadInfoBarDelegate())));
}

BraveShieldsReloadInfoBarDelegate::BraveShieldsReloadInfoBarDelegate()
    : ConfirmInfoBarDelegate() {}

BraveShieldsReloadInfoBarDelegate::~BraveShieldsReloadInfoBarDelegate() =
    default;

infobars::InfoBarDelegate::InfoBarIdentifier
BraveShieldsReloadInfoBarDelegate::GetIdentifier() const {
  return BRAVE_SHIELDS_RELOAD_INFOBAR_DELEGATE;
}

const gfx::VectorIcon& BraveShieldsReloadInfoBarDelegate::GetVectorIcon()
    const {
  return views::kInfoOldIcon;
}

std::u16string BraveShieldsReloadInfoBarDelegate::GetMessageText() const {
  return l10n_util::GetStringUTF16(IDS_PAGE_INFO_INFOBAR_TEXT);
}

int BraveShieldsReloadInfoBarDelegate::GetButtons() const {
  return BUTTON_OK;
}

std::u16string BraveShieldsReloadInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  DCHECK_EQ(BUTTON_OK, button);
  return l10n_util::GetStringUTF16(IDS_PAGE_INFO_INFOBAR_BUTTON);
}

bool BraveShieldsReloadInfoBarDelegate::Accept() {
  content::WebContents* web_contents =
      infobars::ContentInfoBarManager::WebContentsFromInfoBar(infobar());
  if (web_contents) {
    web_contents->GetController().Reload(content::ReloadType::NORMAL, true);
  }
  return true;
}
