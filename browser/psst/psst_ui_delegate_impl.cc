// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_ui_delegate_impl.h"

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "brave/components/psst/core/browser/pref_names.h"
#include "brave/components/psst/core/common/psst_metadata_schema.h"
#include "components/prefs/pref_service.h"

namespace {

constexpr int kMaxPsstInfobarShownCounter = 2;

}  // namespace

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
    url::Origin origin,
    PsstWebsiteSettings dialog_data,
    const int rule_version,
    std::optional<UserScriptResult> user_script_result,
    PsstTabWebContentsObserver::ConsentCallback apply_changes_callback) {
  apply_changes_callback_ = std::move(apply_changes_callback);
  dialog_data_ = std::move(dialog_data);
  origin_ = std::move(origin);
  user_script_result_ = std::move(user_script_result);

  auto icon_status = LocationBarIconStatus::kOnlyIcon;

  // Show icon with badge only if it is initial execution and version changed
  if (user_script_result->initial_execution.has_value() &&
      user_script_result->initial_execution.value()) {
    if ((dialog_data_->consent_status == ConsentStatus::kAsk ||
         dialog_data_->consent_status == ConsentStatus::kAllow) &&
        dialog_data_->script_version != rule_version) {
      icon_status = LocationBarIconStatus::kIconWithBadge;
      // A new version will be saved when the user accepts the consent dialog.
      dialog_data_->script_version = rule_version;
    }
  }

  ui_presenter_->SetLocationBarIconStatus(
      icon_status,
      base::BindOnce(&PsstUiDelegateImpl::OnDontShowForThisSite,
                     weak_ptr_factory_.GetWeakPtr()),
      base::BindOnce(&PsstUiDelegateImpl::OnDisablePrivacySettingsTuning,
                     weak_ptr_factory_.GetWeakPtr()));

  const int infobar_shown_count =
      prefs_->GetInteger(prefs::kPsstInfobarShownCounter);
  // Show the infobar only if it has been shown less than the max allowed times.
  if (infobar_shown_count < kMaxPsstInfobarShownCounter) {
    prefs_->SetInteger(prefs::kPsstInfobarShownCounter,
                       infobar_shown_count + 1);
    ui_presenter_->ShowInfoBar(
        base::BindOnce(&PsstUiDelegateImpl::OnUserAcceptedInfobar,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

void PsstUiDelegateImpl::UpdateTasks(
    long progress,
    const std::vector<PolicyTask>& performed_tasks,
    const mojom::PsstStatus status) {
  if (status == mojom::PsstStatus::kFailed) {
    ui_presenter_->SetLocationBarIconStatus(LocationBarIconStatus::kHidden,
                                            base::NullCallback(),
                                            base::NullCallback());
  }

  if (!ui_presenter_->IsDialogShown()) {
    return;
  }

  // Implementation for setting the current progress.
  for (Observer& obs : observer_list_) {
    if (performed_tasks.empty() && progress == 100) {
      // Update common dialog status
      obs.OnSetRequestStatus("", "");
    } else {
      // Update individual task statuses.
      for (const PolicyTask& task : performed_tasks) {
        obs.OnSetRequestStatus(task.uid, task.error_description);
      }
    }
  }
}

std::optional<PsstWebsiteSettings> PsstUiDelegateImpl::GetPsstWebsiteSettings(
    const url::Origin& origin,
    const std::string& user_id) {
  return psst_settings_service_->GetPsstWebsiteSettings(origin, user_id);
}

void PsstUiDelegateImpl::AddObserver(Observer* obs) {
  observer_list_.AddObserver(obs);
}
void PsstUiDelegateImpl::RemoveObserver(Observer* obs) {
  observer_list_.RemoveObserver(obs);
}
base::WeakPtr<PsstUiDelegateImpl> PsstUiDelegateImpl::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

psst::mojom::SettingCardDataPtr PsstUiDelegateImpl::GetShowDialogData() {
  if (!origin_ || !user_script_result_.has_value() ||
      user_script_result_->tasks.empty()) {
    return nullptr;
  }

  std::vector<mojom::SettingCardDataItemPtr> items;
  for (const auto& task : user_script_result_->tasks) {
    items.push_back(
        psst::mojom::SettingCardDataItem::New(task.description, task.uid));
  }

  return psst::mojom::SettingCardData::New(origin_->GetURL().spec(),
                                           std::move(items));
}

void PsstUiDelegateImpl::OnUserAcceptedPsstSettings(
    const std::vector<std::string>& perform_for_uids) {
  CHECK(origin_);
  CHECK(dialog_data_);
  base::ListValue perform_for_uids_list;
  for (const auto& item : perform_for_uids) {
    perform_for_uids_list.Append(item);
  }
  // Save the PSST settings when user accepts the dialog
  psst_settings_service_->SetPsstWebsiteSettings(
      origin_.value(), ConsentStatus::kAllow, dialog_data_->script_version,
      dialog_data_->user_id, std::move(perform_for_uids_list));

  if (apply_changes_callback_) {
    std::move(apply_changes_callback_).Run(perform_for_uids);
  }
}

void PsstUiDelegateImpl::OnUserAcceptedInfobar(const bool is_accepted) {
  // Handle the user's response to the infobar
  if (is_accepted) {
    // Hide notification badge on the icon after the user interacts with the
    // infobar
    ui_presenter_->SetLocationBarIconStatus(
        LocationBarIconStatus::kOnlyIcon,
        base::BindOnce(&PsstUiDelegateImpl::OnDontShowForThisSite,
                       weak_ptr_factory_.GetWeakPtr()),
        base::BindOnce(&PsstUiDelegateImpl::OnDisablePrivacySettingsTuning,
                       weak_ptr_factory_.GetWeakPtr()));

    ui_presenter_->ShowConsentDialog();
  } else {
    // Disable PSST if user declined the infobar
    prefs_->SetBoolean(prefs::kPsstEnabled, false);
  }
}

void PsstUiDelegateImpl::OnDontShowForThisSite() {
  CHECK(origin_);
  CHECK(dialog_data_);
  psst_settings_service_->SetPsstWebsiteSettings(
      origin_.value(), ConsentStatus::kBlock, dialog_data_->script_version,
      dialog_data_->user_id, {});
  ui_presenter_->HideInfoBar();
  ui_presenter_->SetLocationBarIconStatus(LocationBarIconStatus::kHidden,
                                          base::NullCallback(),
                                          base::NullCallback());
}

void PsstUiDelegateImpl::OnDisablePrivacySettingsTuning() {
  prefs_->SetBoolean(prefs::kPsstEnabled, false);
  ui_presenter_->HideInfoBar();
  ui_presenter_->SetLocationBarIconStatus(LocationBarIconStatus::kHidden,
                                          base::NullCallback(),
                                          base::NullCallback());
}

}  // namespace psst
