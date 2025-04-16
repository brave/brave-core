/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/psst/psst_dialog_tab_helper_delegate_impl.h"

#include "brave/browser/ui/webui/psst/brave_psst_dialog.h"

namespace psst {

PsstDialogTabHelperDelegateImpl::PsstDialogTabHelperDelegateImpl() = default;

PsstDialogTabHelperDelegateImpl::~PsstDialogTabHelperDelegateImpl() = default;

void PsstDialogTabHelperDelegateImpl::ShowPsstConsentDialog(
    content::WebContents* contents,
    std::unique_ptr<ShowDialogData> show_dialog_data) {
  PsstDialogDelegate::ShowPsstConsentDialog(
      contents, std::move(show_dialog_data));
  psst::OpenPsstDialog(contents);
}

void PsstDialogTabHelperDelegateImpl::SetProgressValue(
    content::WebContents* contents,
    const double value) {
  NOTIMPLEMENTED();
}

void PsstDialogTabHelperDelegateImpl::SetRequestDone(
    content::WebContents* contents,
    const std::string& url,
    const std::optional<std::string>& error) {
  PsstDialogDelegate::SetRequestDone(
      contents, url, error);
}

void PsstDialogTabHelperDelegateImpl::SetCompletedView(
    content::WebContents* contents,
    const std::vector<std::string>& applied_checks,
    const std::vector<std::string>& errors,
    ShareCallback share_cb) {
  PsstDialogDelegate::SetCompletedView(
        contents, applied_checks, errors, std::move(share_cb));
}

void PsstDialogTabHelperDelegateImpl::Close(content::WebContents* contents) {
  NOTIMPLEMENTED();
}

}  // namespace psst
