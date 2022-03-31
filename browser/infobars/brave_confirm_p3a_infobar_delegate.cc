/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/brave_confirm_p3a_infobar_delegate.h"

#include <memory>
#include <utility>

#include "brave/common/pref_names.h"
#include "brave/common/url_constants.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/infobars/confirm_infobar_creator.h"
#include "chrome/grit/chromium_strings.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/vector_icons.h"

// static
void BraveConfirmP3AInfoBarDelegate::Create(
    infobars::ContentInfoBarManager* infobar_manager,
    PrefService* local_state) {
  // Don't show infobar if:
  // - P3A is disabled
  // - notice has already been acknowledged
  if (local_state) {
    if (!local_state->GetBoolean(brave::kP3AEnabled) ||
        local_state->GetBoolean(brave::kP3ANoticeAcknowledged)) {
      local_state->SetBoolean(brave::kP3ANoticeAcknowledged, true);
      return;
    }
  }

  infobar_manager->AddInfoBar(
      CreateConfirmInfoBar(std::unique_ptr<ConfirmInfoBarDelegate>(
          new BraveConfirmP3AInfoBarDelegate(local_state))));
}

BraveConfirmP3AInfoBarDelegate::BraveConfirmP3AInfoBarDelegate(
    PrefService* local_state)
    : ConfirmInfoBarDelegate(), local_state_(local_state) {}

BraveConfirmP3AInfoBarDelegate::~BraveConfirmP3AInfoBarDelegate() {}

infobars::InfoBarDelegate::InfoBarIdentifier
BraveConfirmP3AInfoBarDelegate::GetIdentifier() const {
  return BRAVE_CONFIRM_P3A_INFOBAR_DELEGATE;
}

const gfx::VectorIcon& BraveConfirmP3AInfoBarDelegate::GetVectorIcon() const {
  return views::kInfoIcon;
}

bool BraveConfirmP3AInfoBarDelegate::ShouldExpire(
    const NavigationDetails& details) const {
  return false;
}

void BraveConfirmP3AInfoBarDelegate::InfoBarDismissed() {
  // Mark notice as acknowledged when infobar is dismissed
  if (local_state_) {
    local_state_->SetBoolean(brave::kP3ANoticeAcknowledged, true);
  }
}

std::u16string BraveConfirmP3AInfoBarDelegate::GetMessageText() const {
  return l10n_util::GetStringUTF16(IDS_BRAVE_CONFIRM_P3A_INFO_BAR);
}

int BraveConfirmP3AInfoBarDelegate::GetButtons() const {
  return BUTTON_OK | BUTTON_CANCEL;
}

std::u16string BraveConfirmP3AInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  if (button == BUTTON_CANCEL) {
    return l10n_util::GetStringUTF16(IDS_BRAVE_CONFIRM_P3A_INFO_BAR_DISABLE);
  }
  return l10n_util::GetStringUTF16(IDS_BRAVE_CONFIRM_P3A_INFO_BAR_ACKNOWLEDGE);
}

std::u16string BraveConfirmP3AInfoBarDelegate::GetLinkText() const {
  return l10n_util::GetStringUTF16(IDS_LEARN_MORE);
}

GURL BraveConfirmP3AInfoBarDelegate::GetLinkURL() const {
  return GURL(kP3ALearnMoreURL);
}

bool BraveConfirmP3AInfoBarDelegate::Accept() {
  // Mark notice as acknowledged when infobar is dismissed
  if (local_state_) {
    local_state_->SetBoolean(brave::kP3ANoticeAcknowledged, true);
  }
  return true;
}

bool BraveConfirmP3AInfoBarDelegate::Cancel() {
  // OK button is "Disable"
  // Clicking should disable P3A
  if (local_state_) {
    local_state_->SetBoolean(brave::kP3AEnabled, false);
  }
  return true;
}
