// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_ui_delegate_impl.h"

#include "content/public/browser/web_contents.h"

namespace psst {

PsstUiDelegateImpl::PsstUiDelegateImpl(
    BravePsstPermissionContext* permission_context,
    content::WebContents* contents)
    : web_contents_(contents), permission_context_(permission_context) {
  CHECK(permission_context);
}

PsstUiDelegateImpl::~PsstUiDelegateImpl() = default;

void PsstUiDelegateImpl::Show(PsstConsentData dialog_data) {
  dialog_data_ = std::move(dialog_data);

  // Implementation for showing the consent dialog to the user.

  // When dialog accepted by the user
  OnUserAcceptedPsstSettings(base::Value::List());
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

void PsstUiDelegateImpl::OnUserAcceptedPsstSettings(
    base::Value::List urls_to_skip) {
  // Create the PSST permission when user accepts the dialog
  permission_context_->GrantPermission(
      url::Origin::Create(web_contents_->GetLastCommittedURL()),
      ConsentStatus::kAllow, dialog_data_->script_version,
      dialog_data_->user_id, urls_to_skip.Clone());

  if (dialog_data_->apply_changes_callback) {
    std::move(dialog_data_->apply_changes_callback)
        .Run(std::move(urls_to_skip));
  }
}

}  // namespace psst
