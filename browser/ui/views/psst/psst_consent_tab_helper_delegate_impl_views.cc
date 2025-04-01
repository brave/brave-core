/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 #include "brave/browser/psst/psst_consent_tab_helper_delegate_impl.h"

 #include "base/functional/bind.h"
#include "base/values.h"
#include "brave/browser/ui/views/psst/psst_consent_dialog.h"
#include "brave/browser/ui/views/psst/psst_consent_dialog_tracker.h"
#include "components/constrained_window/constrained_window_views.h"
#include "content/public/browser/web_contents.h"

namespace {

PsstConsentDialog* GetDelegate(content::WebContents* contents) {
  auto* dialog_tracker = PsstConsentDialogTracker::FromWebContents(contents);
  if (!dialog_tracker || !dialog_tracker->active_dialog() ||
      !dialog_tracker->active_dialog()->widget_delegate()) {
    return nullptr;
  }

  return static_cast<PsstConsentDialog*>(
      dialog_tracker->active_dialog()->widget_delegate());
}

void OnConsentCallback(psst::PsstTabHelper::Delegate::ConsentCallback cb,
                       const std::vector<std::string>& skipped_checks) {
  std::move(cb).Run(skipped_checks);
}

}  // namespace

void PsstConsentTabHelperDelegateImpl::ShowPsstConsentDialog(
    content::WebContents* contents,
    bool prompt_for_new_version,
    base::Value::List requests,
    ConsentCallback yes_cb,
    ConsentCallback no_cb,
    base::OnceClosure never_ask_me_callback) {
  PsstConsentDialogTracker::CreateForWebContents(contents);
  auto* dialog_tracker = PsstConsentDialogTracker::FromWebContents(contents);
  if (!dialog_tracker) {
    return;
  }

  auto* new_dialog = constrained_window::ShowWebModalDialogViews(
      new PsstConsentDialog(
          prompt_for_new_version, std::move(requests),
          base::BindOnce(&OnConsentCallback, std::move(yes_cb)),
          base::BindOnce(&OnConsentCallback, std::move(no_cb),
                         std::vector<std::string>{}),
          std::move(never_ask_me_callback)),
      contents);
  dialog_tracker->SetActiveDialog(new_dialog);

  new_dialog->Show();
}

void PsstConsentTabHelperDelegateImpl::SetProgressValue(
    content::WebContents* contents,
    const double value) {
  auto* delegate = GetDelegate(contents);
  if (!delegate) {
    return;
  }
  delegate->SetProgressValue(std::move(value));
}

void PsstConsentTabHelperDelegateImpl::SetRequestDone(
    content::WebContents* contents,
    const std::string& url,
    const bool is_error) {
  auto* delegate = GetDelegate(contents);
  if (!delegate) {
    return;
  }

  delegate->SetRequestDone(url, is_error);
}

void PsstConsentTabHelperDelegateImpl::SetCompletedView(
    content::WebContents* contents,
    const std::vector<std::string>& applied_checks,
    const std::vector<std::string>& errors,
    ShareCallback share_cb) {
  auto* delegate = GetDelegate(contents);
  if (!delegate) {
    return;
  }

  delegate->SetCompletedView(applied_checks, errors, std::move(share_cb));
}

void PsstConsentTabHelperDelegateImpl::Close(content::WebContents* contents) {
  auto* delegate = GetDelegate(contents);
  if (!delegate || !delegate->GetWidget()) {
    return;
  }
  delegate->GetWidget()->CloseWithReason(
      views::Widget::ClosedReason::kCancelButtonClicked);
}
