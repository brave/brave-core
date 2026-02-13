// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_ui_delegate_impl.h"

#include "base/functional/bind.h"
#include "brave/browser/psst/psst_ui_presenter.h"
#include "brave/components/psst/common/psst_metadata_schema.h"

namespace psst {

namespace {

std::vector<std::string> ListValueToStringVector(
    const base::ListValue& list_value) {
  std::vector<std::string> result;
  for (const auto& value : list_value) {
    if (value.is_string()) {
      result.push_back(value.GetString());
    }
  }
  return result;
}

}  // namespace

PsstUiDelegateImpl::PsstUiDelegateImpl(
    PsstSettingsService* psst_settings_service,
    std::unique_ptr<PsstUiPresenter> ui_presenter)
    : ui_presenter_(std::move(ui_presenter)),
      psst_settings_service_(psst_settings_service) {
  CHECK(psst_settings_service_);
  CHECK(ui_presenter_);
}
PsstUiDelegateImpl::~PsstUiDelegateImpl() = default;

void PsstUiDelegateImpl::Show(
    const url::Origin& origin,
    PsstWebsiteSettings dialog_data,
    const std::string& site_name,
    base::ListValue tasks,
    PsstTabWebContentsObserver::ConsentCallback apply_changes_callback) {
  auto psst_settings = psst_settings_service_->GetPsstWebsiteSettings(
      origin, dialog_data.user_id);

  if (psst_settings &&
      psst_settings->consent_status == psst::ConsentStatus::kBlock) {
    return;  // Do nothing if the user has blocked the PSST for that site
  }

  apply_changes_callback_ = std::move(apply_changes_callback);
  dialog_data_ = std::move(dialog_data);

  // Implementation for showing the consent dialog to the user.
  ui_presenter_->ShowIcon();

  ui_presenter_->ShowInfoBar(base::BindOnce(
      &PsstUiDelegateImpl::OnUserAcceptedInfobar, weak_ptr_factory_.GetWeakPtr(),
      origin));
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
    std::move(apply_changes_callback_)
        .Run(ListValueToStringVector(std::move(urls_to_skip)));
  }
}

void PsstUiDelegateImpl::OnUserAcceptedInfobar(const url::Origin& origin, const bool is_accepted) {
  // Handle the user's response to the infobar
  if (is_accepted) {
    // User accepted the infobar, save info in preferences that infobar is accepted
  } else {
    // User declined the infobar
  }
}

}  // namespace psst
