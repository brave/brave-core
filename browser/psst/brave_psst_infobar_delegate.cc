// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/brave_psst_infobar_delegate.h"

#include <memory>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/infobars/confirm_infobar_creator.h"
#include "components/infobars/core/infobar.h"
#include "ui/base/l10n/l10n_util.h"

namespace psst {
// static
void BravePsstInfoBarDelegate::Create(infobars::InfoBarManager* infobar_manager,
                                      AcceptCallback on_accept_callback) {
  infobar_manager->AddInfoBar(
      CreateConfirmInfoBar(base::WrapUnique<BravePsstInfoBarDelegate>(
          new BravePsstInfoBarDelegate(std::move(on_accept_callback)))));
}

BravePsstInfoBarDelegate::BravePsstInfoBarDelegate(
    AcceptCallback on_accept_callback)
    : on_accept_callback_(std::move(on_accept_callback)) {}

BravePsstInfoBarDelegate::~BravePsstInfoBarDelegate() = default;

infobars::InfoBarDelegate::InfoBarIdentifier
BravePsstInfoBarDelegate::GetIdentifier() const {
  return BRAVE_PSST_INFOBAR_DELEGATE;
}

std::u16string BravePsstInfoBarDelegate::GetMessageText() const {
  return l10n_util::GetStringUTF16(IDS_BRAVE_PSST_INFOBAR_MESSAGE);
}

int BravePsstInfoBarDelegate::GetButtons() const {
  return BUTTON_OK | BUTTON_CANCEL;
}

std::u16string BravePsstInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  if (button == BUTTON_OK) {
    return l10n_util::GetStringUTF16(
        IDS_BRAVE_PSST_INFO_BAR_REVIEW_SUGGESTIONS);
  }

  if (button == BUTTON_CANCEL) {
    return l10n_util::GetStringUTF16(
        IDS_BRAVE_PSST_INFO_BAR_REVIEW_SUGGESTIONS_CANCEL);
  }

  return std::u16string();
}

bool BravePsstInfoBarDelegate::Accept() {
  if (on_accept_callback_) {
    std::move(on_accept_callback_).Run(true);
  }

  return true;
}

bool BravePsstInfoBarDelegate::Cancel() {
  if (on_accept_callback_) {
    std::move(on_accept_callback_).Run(false);
  }

  return true;
}

}  // namespace psst
