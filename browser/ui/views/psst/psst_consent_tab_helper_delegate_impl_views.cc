/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
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
  if(!dialog_tracker) {
    return nullptr;
  }

  return static_cast<PsstConsentDialog*>(dialog_tracker->active_dialog()->widget_delegate());
}

}  // namespace

void PsstConsentTabHelperDelegateImpl::ShowPsstConsentDialog(
    content::WebContents* contents,
    bool prompt_for_new_version,
    base::Value::List requests,
    base::OnceClosure yes_cb,
    base::OnceClosure no_cb) {
  PsstConsentDialogTracker::CreateForWebContents(contents);
  auto* dialog_tracker = PsstConsentDialogTracker::FromWebContents(contents);
  if(!dialog_tracker) {
    return;
  }

  auto* new_dialog = constrained_window::ShowWebModalDialogViews(
new PsstConsentDialog(prompt_for_new_version, std::move(requests),
                            std::move(yes_cb), std::move(no_cb)),
      contents);
  dialog_tracker->SetActiveDialog(new_dialog);

  new_dialog->Show();
}

void PsstConsentTabHelperDelegateImpl::SetProgressValue(content::WebContents* contents, const double value) {
  auto* delegate = GetDelegate(contents);
  if(!delegate) {
    return;
  }

  delegate->SetProgressValue(std::move(value));
}

void PsstConsentTabHelperDelegateImpl::SetRequestDone(content::WebContents* contents, const std::string& url) {
  auto* delegate = GetDelegate(contents);
  if(!delegate) {
    return;
  }

  delegate->SetRequestDone(url);
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
