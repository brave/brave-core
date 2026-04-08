// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_ui_delegate_impl.h"

#include "base/strings/string_util.h"
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
    url::Origin origin,
    PsstWebsiteSettings dialog_data,
    std::optional<UserScriptResult> user_script_result,
    PsstTabWebContentsObserver::ConsentCallback apply_changes_callback) {
  apply_changes_callback_ = std::move(apply_changes_callback);
  dialog_data_ = std::move(dialog_data);
  origin_ = std::move(origin);
  user_script_result_ = std::move(user_script_result);
  ui_presenter_->ShowInfoBar(
      base::BindOnce(&PsstUiDelegateImpl::OnUserAcceptedInfobar,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PsstUiDelegateImpl::UpdateTasks(
    long progress,
    const std::vector<PolicyTask>& applied_tasks,
    const mojom::PsstStatus status) {
  // Implementation for setting the current progress.
  for (Observer& obs : observer_list_) {
    std::ranges::for_each(applied_tasks, [&obs](const PolicyTask& task) {
      obs.OnSetRequestDone(task.url, task.error_description);
    });
  }

  if (status != mojom::PsstStatus::kCompleted) {
    return;
  }

  std::vector<std::string> applied_list;
  std::vector<std::string> failed_list;
  std::ranges::for_each(
      applied_tasks, [&applied_list, &failed_list](const PolicyTask& task) {
        (!task.error_description.has_value() || task.error_description->empty())
            ? applied_list.emplace_back(task.url)
            : failed_list.emplace_back(task.url);
      });
  for (Observer& obs : observer_list_) {
    obs.OnSetCompleted(applied_list, failed_list);
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
bool PsstUiDelegateImpl::HasObserver(Observer* observer) {
  return observer_list_.HasObserver(observer);
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
        psst::mojom::SettingCardDataItem::New(task.description, task.url));
  }

  return psst::mojom::SettingCardData::New(origin_->GetURL().spec(),
                                           std::move(items));
}

void PsstUiDelegateImpl::OnUserAcceptedPsstSettings(
    const url::Origin& origin,
    const std::vector<std::string>& urls_to_skip) {
  base::ListValue urls_to_skip_list;
  for (const auto& item : urls_to_skip) {
    urls_to_skip_list.Append(item);
  }
  // Save the PSST settings when user accepts the dialog
  psst_settings_service_->SetPsstWebsiteSettings(
      origin, ConsentStatus::kAllow, dialog_data_->script_version,
      dialog_data_->user_id, std::move(urls_to_skip_list));

  if (apply_changes_callback_) {
    std::move(apply_changes_callback_).Run(urls_to_skip);
  }
}

void PsstUiDelegateImpl::OnUserAcceptedInfobar(const bool is_accepted) {
  // Handle the user's response to the infobar
  if (is_accepted) {
    ui_presenter_->ShowConsentDialog();
  } else {
    // Disable PSST if user declined the infobar
    prefs_->SetBoolean(prefs::kPsstEnabled, false);
  }
}

}  // namespace psst
