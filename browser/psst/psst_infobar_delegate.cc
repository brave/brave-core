// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_infobar_delegate.h"

#include <memory>

#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/task/sequenced_task_runner.h"
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

  // The infobar is already closed by the callback above (it synchronously
  // triggers ShowConsentDialog() -> HideInfoBar()), so returning true here
  // would make the caller remove it a second time (use-after-free).
  return false;
}

bool PsstInfoBarDelegate::Cancel() {
  if (!on_accept_callback_.is_null()) {
    std::move(on_accept_callback_).Run(false);
  }

  return true;
}

void PsstInfoBarDelegate::InfoBarDismissed() {
  if (!on_accept_callback_.is_null()) {
    // Unlike Accept()/Cancel(), the caller (InfoBarView::CloseButtonPressed())
    // always calls RemoveSelf() right after this returns, with no way to
    // signal "already removed". The callback below leads (synchronously, if
    // run inline) to SetPsstEnabled(false) -> OnPsstEnableChange() ->
    // HideInfoBar(), which would remove this same infobar a second time
    // while CloseButtonPressed() is still on the stack, causing a
    // use-after-free. Post it instead so it runs after RemoveSelf() has
    // already completed; HideInfoBar() then just no-ops (infobar not found).
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(on_accept_callback_), false));
  }
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

}  // namespace psst
