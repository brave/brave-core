/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/browser/ui/startup/brave_obsolete_system_infobar_delegate.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/infobars/confirm_infobar_creator.h"
#include "chrome/common/pref_names.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"

// static
void BraveObsoleteSystemInfoBarDelegate::Create(
    infobars::ContentInfoBarManager* infobar_manager) {
  infobar_manager->AddInfoBar(
      CreateConfirmInfoBar(std::unique_ptr<ConfirmInfoBarDelegate>(
          new BraveObsoleteSystemInfoBarDelegate())));
}

BraveObsoleteSystemInfoBarDelegate::BraveObsoleteSystemInfoBarDelegate() =
    default;
BraveObsoleteSystemInfoBarDelegate::~BraveObsoleteSystemInfoBarDelegate() =
    default;

int BraveObsoleteSystemInfoBarDelegate::GetButtons() const {
  return BUTTON_OK;
}

std::u16string BraveObsoleteSystemInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  return l10n_util::GetStringUTF16(
      IDS_OBSOLERE_SYSTEM_INFOBAR_DONT_SHOW_BUTTON);
}

bool BraveObsoleteSystemInfoBarDelegate::Accept() {
  if (PrefService* local_state = g_browser_process->local_state()) {
    local_state->SetBoolean(prefs::kSuppressUnsupportedOSWarning, true);
  }
  return true;
}
