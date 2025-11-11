// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_ui_delegate_impl.h"

#include "brave/components/psst/browser/core/brave_psst_utils.h"
#include "brave/components/psst/common/psst_metadata_schema.h"

namespace psst {

PsstUiDelegateImpl::PsstUiDelegateImpl(
    HostContentSettingsMap* host_content_settings_map)
    : host_content_settings_map_(host_content_settings_map) {
  CHECK(host_content_settings_map_);
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

std::optional<PsstMetadata> PsstUiDelegateImpl::GetPsstMetadata(
    const url::Origin& origin,
    const std::string& user_id) {
  return psst::GetPsstMetadata(host_content_settings_map_, origin, user_id);
}

void PsstUiDelegateImpl::OnUserAcceptedPsstSettings(
    base::Value::List urls_to_skip) {
  // Create the PSST permission when user accepts the dialog
  SetPsstMetadata(host_content_settings_map_, dialog_data_->origin,
                  ConsentStatus::kAllow, dialog_data_->script_version,
                  dialog_data_->user_id, std::move(urls_to_skip));

  if (dialog_data_->apply_changes_callback) {
    std::move(dialog_data_->apply_changes_callback)
        .Run(std::move(urls_to_skip));
  }
}

}  // namespace psst
