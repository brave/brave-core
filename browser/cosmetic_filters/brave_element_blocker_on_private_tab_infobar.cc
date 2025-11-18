/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/cosmetic_filters/brave_element_blocker_on_private_tab_infobar.h"

#include <memory>

#include "base/check.h"
#include "brave/browser/cosmetic_filters/cosmetic_filters_tab_helper.h"
#include "chrome/browser/infobars/confirm_infobar_creator.h"
#include "components/grit/brave_components_strings.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/vector_icons.h"

// static
void BraveElementBlockerOnPrivateTabInfoBarDelegate::Create(
    infobars::ContentInfoBarManager* infobar_manager,
    content::WebContents* source_web_contents) {
  CHECK(infobar_manager);
  CHECK(source_web_contents);
  infobar_manager->AddInfoBar(
      CreateConfirmInfoBar(std::unique_ptr<ConfirmInfoBarDelegate>(
          new BraveElementBlockerOnPrivateTabInfoBarDelegate(
              source_web_contents))));
}

BraveElementBlockerOnPrivateTabInfoBarDelegate::
    BraveElementBlockerOnPrivateTabInfoBarDelegate(
        content::WebContents* source_web_contents)
    : source_web_contents_(source_web_contents) {}
BraveElementBlockerOnPrivateTabInfoBarDelegate::
    ~BraveElementBlockerOnPrivateTabInfoBarDelegate() = default;

infobars::InfoBarDelegate::InfoBarIdentifier
BraveElementBlockerOnPrivateTabInfoBarDelegate::GetIdentifier() const {
  return BRAVE_ELEMENT_BLOCKER_ON_PRIVATE_TAB_INFOBAR;
}

const gfx::VectorIcon&
BraveElementBlockerOnPrivateTabInfoBarDelegate::GetVectorIcon() const {
  return views::kInfoIcon;
}

bool BraveElementBlockerOnPrivateTabInfoBarDelegate::ShouldExpire(
    const NavigationDetails& details) const {
  return false;
}

void BraveElementBlockerOnPrivateTabInfoBarDelegate::InfoBarDismissed() {}

std::u16string BraveElementBlockerOnPrivateTabInfoBarDelegate::GetMessageText()
    const {
  return l10n_util::GetStringUTF16(
      IDS_BRAVE_SHIELDS_ALLOW_ELEMENT_BLOCKER_IN_PRIVATE_LABEL_SUBLABEL);
}

int BraveElementBlockerOnPrivateTabInfoBarDelegate::GetButtons() const {
  return BUTTON_OK | BUTTON_CANCEL;
}

bool BraveElementBlockerOnPrivateTabInfoBarDelegate::Accept() {
  cosmetic_filters::CosmeticFiltersTabHelper::LaunchContentPicker(
      source_web_contents_);
  return true;
}

bool BraveElementBlockerOnPrivateTabInfoBarDelegate::Cancel() {
  return true;
}
