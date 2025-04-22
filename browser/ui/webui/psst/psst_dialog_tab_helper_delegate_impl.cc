/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/psst/psst_dialog_tab_helper_delegate_impl.h"

#include "brave/browser/ui/webui/psst/brave_psst_dialog.h"

namespace psst {

PsstDialogTabHelperDelegateImpl::PsstDialogTabHelperDelegateImpl(
    content::WebContents* contents)
    : web_contents_(contents) {}

PsstDialogTabHelperDelegateImpl::~PsstDialogTabHelperDelegateImpl() = default;

void PsstDialogTabHelperDelegateImpl::ShowPsstConsentDialog(
    std::unique_ptr<ShowDialogData>& show_dialog_data) {
  PsstDialogDelegate::ShowPsstConsentDialog(show_dialog_data);
  psst::OpenPsstDialog(web_contents_);
}

}  // namespace psst
