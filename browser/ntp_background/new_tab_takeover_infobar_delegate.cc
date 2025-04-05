/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ntp_background/new_tab_takeover_infobar_delegate.h"

#include <memory>

#include "brave/components/ntp_background_images/common/new_tab_takeover_infobar_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/infobars/confirm_infobar_creator.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "url/gurl.h"

namespace ntp_background_images {

namespace {
constexpr char kLearnMoreUrl[] =
    "https://support.brave.com/hc/en-us/articles/35182999599501";
}  // namespace

// static
void NewTabTakeoverInfoBarDelegate::MaybeCreate(
    content::WebContents* web_contents,
    PrefService* prefs) {
  CHECK(web_contents);
  CHECK(prefs);

  if (!ShouldShowNewTabTakeoverInfobar(prefs)) {
    return;
  }
  RecordNewTabTakeoverInfobarWasShown(prefs);

  if (infobars::ContentInfoBarManager* infobar_manager =
          infobars::ContentInfoBarManager::FromWebContents(web_contents)) {
    infobar_manager->AddInfoBar(CreateConfirmInfoBar(
        std::make_unique<NewTabTakeoverInfoBarDelegate>(prefs)));
  }
}

NewTabTakeoverInfoBarDelegate::NewTabTakeoverInfoBarDelegate(PrefService* prefs)
    : prefs_(prefs) {}

NewTabTakeoverInfoBarDelegate::~NewTabTakeoverInfoBarDelegate() = default;

infobars::InfoBarDelegate::InfoBarIdentifier
NewTabTakeoverInfoBarDelegate::GetIdentifier() const {
  return NEW_TAB_TAKEOVER_INFOBAR_DELEGATE;
}

ui::ImageModel NewTabTakeoverInfoBarDelegate::GetIcon() const {
  return ui::ImageModel::FromVectorIcon(vector_icons::kProductIcon,
                                        ui::kColorIcon, /*icon_size=*/20);
}

std::u16string NewTabTakeoverInfoBarDelegate::GetMessageText() const {
  return l10n_util::GetStringUTF16(IDS_NEW_TAB_TAKEOVER_INFOBAR_MESSAGE);
}

int NewTabTakeoverInfoBarDelegate::GetButtons() const {
  return BUTTON_NONE;
}

std::u16string NewTabTakeoverInfoBarDelegate::GetLinkText() const {
  return l10n_util::GetStringUTF16(
      IDS_NEW_TAB_TAKEOVER_INFOBAR_LEARN_MORE_OPT_OUT_CHOICES);
}

GURL NewTabTakeoverInfoBarDelegate::GetLinkURL() const {
  return GURL(kLearnMoreUrl);
}

bool NewTabTakeoverInfoBarDelegate::LinkClicked(
    WindowOpenDisposition disposition) {
  SuppressNewTabTakeoverInfobar(prefs_);
  ConfirmInfoBarDelegate::LinkClicked(disposition);
  // Return true to immediately close the infobar.
  return true;
}

void NewTabTakeoverInfoBarDelegate::InfoBarDismissed() {
  SuppressNewTabTakeoverInfobar(prefs_);
}

}  // namespace ntp_background_images
