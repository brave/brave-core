// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_infobar_delegate.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/components/psst/core/common/constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "components/infobars/core/infobar.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/infobars/confirm_infobar_creator.h"
#endif  // BUILDFLAG(IS_ANDROID)

namespace psst {
// static
void PsstInfoBarDelegate::Create(infobars::InfoBarManager* infobar_manager,
                                 AcceptCallback on_accept_callback) {
  auto delegate = base::WrapUnique<PsstInfoBarDelegate>(
      new PsstInfoBarDelegate(std::move(on_accept_callback)));
#if BUILDFLAG(IS_ANDROID)
  infobar_manager->AddInfoBar(CreateConfirmInfoBar(std::move(delegate)));
#else
  infobar_manager->AddInfoBar(CreatePsstInfoBar(std::move(delegate)));
#endif  // BUILDFLAG(IS_ANDROID)
}

void PsstInfoBarDelegate::DisableCallback() {
  on_accept_callback_.Reset();
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

void PsstInfoBarDelegate::InfoBarDismissed() {
  if (!on_accept_callback_.is_null()) {
    std::move(on_accept_callback_).Run(false);
  }
}

infobars::InfoBarDelegate::InfoBarIdentifier
PsstInfoBarDelegate::GetIdentifier() const {
  return BRAVE_PSST_INFOBAR_DELEGATE;
}

ui::ImageModel PsstInfoBarDelegate::GetIcon() const {
  // Drawn on top of a branded badge background (see PsstInfoBar), so it
  // always needs to contrast with that fixed color, not the theme's
  // default icon color.
  return ui::ImageModel::FromVectorIcon(kLeoPsstIcon, nala::kColorPrimitivePrimary30,
                                        /*icon_size=*/32);
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

GURL PsstInfoBarDelegate::GetLinkURL() const {
  return GURL(kPsstReportDialogLearnMoreUrl);
}

}  // namespace psst
