// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_ui_delegate_impl.h"

#include "brave/browser/psst/psst_ui_presenter.h"
namespace psst {

PsstUiDelegateImpl::PsstUiDelegateImpl(
    BravePsstPermissionContext* permission_context, std::unique_ptr<PsstUiPresenter> ui_presenter)
    : permission_context_(permission_context), ui_presenter_(std::move(ui_presenter)) {
  CHECK(permission_context);
}

PsstUiDelegateImpl::~PsstUiDelegateImpl() = default;

void PsstUiDelegateImpl::Show(PsstConsentData dialog_data, permissions::PermissionPrompt::Delegate* delegate) {
  dialog_data_ = std::move(dialog_data);
  delegate_ = delegate;
LOG(INFO) << "[PSST] PsstUiDelegateImpl::Show delegate_: " << (delegate_ ? "not null" : "null");
  // Implementation for showing the consent dialog to the user.

  // When dialog accepted by the user
  //OnUserAcceptedPsstSettings(base::Value::List());
}

void PsstUiDelegateImpl::ShowPsstInfobar(PsstTabWebContentsObserver::InfobarCallback callback, permissions::PermissionPrompt::Delegate* delegate) {
  delegate_ = delegate;

ui_presenter_->ShowInfoBar(
      base::BindOnce(&PsstUiDelegateImpl::OnInfobarAccepted,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PsstUiDelegateImpl::UpdateTasks(
    long progress,
    const std::vector<PolicyTask>& applied_tasks,
    const mojom::PsstStatus status) {
  // Implementation for setting the current progress.
}

std::optional<PsstPermissionInfo> PsstUiDelegateImpl::GetPsstPermissionInfo(
    const url::Origin& origin,
    const std::string& user_id) {
  return permission_context_->GetPsstPermissionInfo(origin, user_id);
}

void PsstUiDelegateImpl::OnInfobarAccepted(PsstTabWebContentsObserver::InfobarCallback callback,
                                           const bool is_accepted) {
LOG(INFO) << "[PSST] OnInfobarAccepted delegate_: " << (delegate_ ? "not null" : "null") << " is_accepted:" << is_accepted;
  // if (!callback) {
  //   return;
  // }

  if(!is_accepted) {
    delegate_->Deny();
    return;
  }

  delegate_->Accept();

//  std::move(callback).Run(is_accepted);
}

void PsstUiDelegateImpl::OnUserAcceptedPsstSettings(
    base::Value::List urls_to_skip) {
LOG(INFO) << "[PSST] User accepted PSST settings";

  // Create the PSST permission when user accepts the dialog
  permission_context_->GrantPermission(
      dialog_data_->origin, ConsentStatus::kAllow, dialog_data_->script_version,
      dialog_data_->user_id, urls_to_skip.Clone());

  if (dialog_data_->apply_changes_callback) {
    std::move(dialog_data_->apply_changes_callback)
        .Run(std::move(urls_to_skip));
  }
}

}  // namespace psst
