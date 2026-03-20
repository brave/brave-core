// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_infobar_delegate.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/infobars/confirm_infobar_creator.h"
#include "components/infobars/core/infobar.h"
#include "ui/base/l10n/l10n_util.h"

namespace psst {
// static
void PsstInfoBarDelegate::Create(infobars::InfoBarManager* infobar_manager,
                                 AcceptCallback on_accept_callback) {
  infobar_manager->AddInfoBar(
      CreateConfirmInfoBar(base::WrapUnique<PsstInfoBarDelegate>(
          new PsstInfoBarDelegate(std::move(on_accept_callback)))));
}

PsstInfoBarDelegate::PsstInfoBarDelegate(AcceptCallback on_accept_callback)
    : on_accept_callback_(std::move(on_accept_callback)) {}

PsstInfoBarDelegate::~PsstInfoBarDelegate() {
  if (!on_accept_callback_.is_null()) {
    std::move(on_accept_callback_).Run(false);
  }
}

bool PsstInfoBarDelegate::Accept() {
  if (!on_accept_callback_.is_null()) {
    std::move(on_accept_callback_).Run(true);
  }

  return true;
}

bool PsstInfoBarDelegate::Cancel() {
  if (!on_accept_callback_.is_null()) {
    std::move(on_accept_callback_).Run(false);
  }

  return true;
}

infobars::InfoBarDelegate::InfoBarIdentifier
PsstInfoBarDelegate::GetIdentifier() const {
  return BRAVE_PSST_INFOBAR_DELEGATE;
}

std::u16string PsstInfoBarDelegate::GetMessageText() const {
  return l10n_util::GetStringUTF16(IDS_BRAVE_PSST_INFOBAR_MESSAGE);
}

int PsstInfoBarDelegate::GetButtons() const {
  return BUTTON_OK;
}

std::u16string PsstInfoBarDelegate::GetButtonLabel(InfoBarButton button) const {
  return l10n_util::GetStringUTF16(IDS_BRAVE_PSST_INFO_BAR_REVIEW_SUGGESTIONS);
}

void PsstInfoBarDelegate::InfoBarDismissed() {
  if (!on_accept_callback_.is_null()) {
    std::move(on_accept_callback_).Run(false);
  }
}

}  // namespace psst
