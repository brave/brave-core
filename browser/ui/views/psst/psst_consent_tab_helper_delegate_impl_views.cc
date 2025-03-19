/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include "base/functional/bind.h"
#include "brave/browser/psst/psst_consent_tab_helper_delegate_impl.h"

#include "base/functional/callback.h"
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

void OnConsentCallback(psst::PsstTabHelper::Delegate::ConsentCallback cb, const std::vector<std::string>& skipped_checks) {
LOG(INFO) << "[PSST] OnConsentCallback skipped_checks.size:" << skipped_checks.size() << " cb.is_null:" << cb.is_null();
std::move(cb).Run(skipped_checks);
}

}  // namespace

void PsstConsentTabHelperDelegateImpl::ShowPsstConsentDialog(
    content::WebContents* contents,
    bool prompt_for_new_version,
    base::Value::List requests,
    ConsentCallback yes_cb,
                                       ConsentCallback no_cb) {
  PsstConsentDialogTracker::CreateForWebContents(contents);
  auto* dialog_tracker = PsstConsentDialogTracker::FromWebContents(contents);
  if(!dialog_tracker) {
    return;
  }

  auto* new_dialog = constrained_window::ShowWebModalDialogViews(
new PsstConsentDialog(prompt_for_new_version, std::move(requests),
                            base::BindOnce(&OnConsentCallback, std::move(yes_cb)), 
                            base::BindOnce(&OnConsentCallback, std::move(no_cb), std::vector<std::string>{})),
      contents);
  dialog_tracker->SetActiveDialog(new_dialog);

  new_dialog->Show();
}

void PsstConsentTabHelperDelegateImpl::SetProgressValue(content::WebContents* contents, const double value) {
  auto* delegate = GetDelegate(contents);
  if(!delegate) {
    return;
  }
LOG(INFO) << "[PSST] SetProgressValue value:" << value;
  delegate->SetProgressValue(std::move(value));
}

void PsstConsentTabHelperDelegateImpl::SetRequestDone(content::WebContents* contents, const std::string& url) {
  auto* delegate = GetDelegate(contents);
  if(!delegate) {
    return;
  }

  delegate->SetRequestDone(url);
}

void PsstConsentTabHelperDelegateImpl::SetRequestError(content::WebContents* contents, const std::string& url, const std::string& error) {
  auto* delegate = GetDelegate(contents);
  if(!delegate) {
    return;
  }

  delegate->SetRequestError(url, error);
}

void PsstConsentTabHelperDelegateImpl::SetCompletedView(content::WebContents* contents, const std::vector<std::string>& applied_checks, const std::vector<std::string>& errors) {
  auto* delegate = GetDelegate(contents);
  if(!delegate) {
    return;
  }

  delegate->SetCompletedView(applied_checks, errors);
}

void PsstConsentTabHelperDelegateImpl::Close(content::WebContents* contents) {
  LOG(INFO) << "[PSST] PsstConsentTabHelperDelegateImpl::Close #100";
  auto* delegate = GetDelegate(contents);
  if(!delegate || !delegate->GetWidget()) {
    LOG(INFO) << "[PSST] PsstConsentTabHelperDelegateImpl::Close #200";
    return;
  }
  delegate->GetWidget()->CloseWithReason(
      views::Widget::ClosedReason::kCancelButtonClicked);
  LOG(INFO) << "[PSST] PsstConsentTabHelperDelegateImpl::Close #300";
}
