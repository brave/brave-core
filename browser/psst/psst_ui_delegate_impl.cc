// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_ui_delegate_impl.h"

#include "brave/components/psst/common/pref_names.h"
#include "brave/components/psst/common/psst_metadata_schema.h"
#include "components/prefs/pref_service.h"

namespace psst {

PsstUiDelegateImpl::PsstUiDelegateImpl(
    PsstSettingsService* psst_settings_service,
    PrefService* prefs,
    std::unique_ptr<PsstUiPresenter> ui_presenter)
    : ui_presenter_(std::move(ui_presenter)),
      psst_settings_service_(psst_settings_service),
      prefs_(prefs) {
  CHECK(psst_settings_service_);
  CHECK(ui_presenter_);
  CHECK(prefs_);
}
PsstUiDelegateImpl::~PsstUiDelegateImpl() = default;

void PsstUiDelegateImpl::Show(
    const url::Origin& origin,
    PsstWebsiteSettings dialog_data,
    PsstTabWebContentsObserver::ConsentCallback apply_changes_callback) {
  apply_changes_callback_ = std::move(apply_changes_callback);
  dialog_data_ = std::move(dialog_data);
  ui_presenter_->ShowInfoBar(
      base::BindOnce(&PsstUiDelegateImpl::OnUserAcceptedInfobar,
                     weak_ptr_factory_.GetWeakPtr(), std::move(origin)));
}

void PsstUiDelegateImpl::UpdateTasks(
    long progress,
    const std::vector<PolicyTask>& applied_tasks,
    const mojom::PsstStatus status) {
  // Implementation for setting the current progress.
}

std::optional<PsstWebsiteSettings> PsstUiDelegateImpl::GetPsstWebsiteSettings(
    const url::Origin& origin,
    const std::string& user_id) {
  return psst_settings_service_->GetPsstWebsiteSettings(origin, user_id);
}

void PsstUiDelegateImpl::OnUserAcceptedPsstSettings(
    const url::Origin& origin,
    base::ListValue urls_to_skip) {
  // Save the PSST settings when user accepts the dialog
  psst_settings_service_->SetPsstWebsiteSettings(
      origin, ConsentStatus::kAllow, dialog_data_->script_version,
      dialog_data_->user_id, urls_to_skip.Clone());

  if (apply_changes_callback_) {
    std::move(apply_changes_callback_).Run(std::move(urls_to_skip));
  }
}

void PsstUiDelegateImpl::OnUserAcceptedInfobar(const url::Origin& origin,
                                               const bool is_accepted) {
  // Handle the user's response to the infobar
  if (is_accepted) {
    // Call to presenter to show the consent dialog

    // When the consent dialog is accepted by the user
    OnUserAcceptedPsstSettings(origin, base::ListValue());
  } else {
    // Disable PSST if user declined the infobar
    prefs_->SetBoolean(prefs::kPsstEnabled, false);
  }
}

}  // namespace psst
