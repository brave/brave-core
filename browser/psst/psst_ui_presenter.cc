// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_ui_presenter.h"

#include "brave/browser/psst/psst_infobar_delegate.h"
#include "brave/components/psst/common/constants.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/infobars/content/content_infobar_manager.h"

namespace psst {

UiDesktopPresenter::UiDesktopPresenter(content::WebContents* web_contents)
    : web_contents_(web_contents) {}
UiDesktopPresenter::~UiDesktopPresenter() = default;

void UiDesktopPresenter::ShowInfoBar(
    PsstInfoBarDelegate::AcceptCallback on_accept_callback) {
  infobars::ContentInfoBarManager* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents_);
  if (!infobar_manager) {
    return;
  }

  PsstInfoBarDelegate::Create(
      infobar_manager, base::BindOnce(&UiDesktopPresenter::OnInfobarAccepted,
                                      weak_ptr_factory_.GetWeakPtr(),
                                      std::move(on_accept_callback)));
}

void UiDesktopPresenter::ShowIcon() {
  // Implementation to show the icon in the UI.
  // This could involve updating the UI state or notifying the user.
}

void UiDesktopPresenter::OnInfobarAccepted(
    PsstInfoBarDelegate::AcceptCallback on_accept_callback,
    const bool is_accepted) {
  if (on_accept_callback) {
    std::move(on_accept_callback).Run(is_accepted);
  }
  // Open the Psst dialog when the infobar is accepted.
  // OpenPsstDialog(web_contents_);
}

}  // namespace psst
