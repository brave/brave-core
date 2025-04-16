/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/browser/core/psst_dialog_delegate.h"

namespace psst {

PsstDialogDelegate::PsstDialogDelegate() = default;
PsstDialogDelegate::~PsstDialogDelegate() = default;

PsstDialogDelegate::ShowDialogData::ShowDialogData(
    const bool is_new_version,
    const std::string& site_name,
    base::Value::List request_infos,
    ConsentCallback apply_changes_callback,
    ConsentCallback cancel_callback,
    base::OnceClosure never_ask_me_callback)
    : is_new_version(is_new_version),
      site_name(site_name),
      request_infos(std::move(request_infos)),
      apply_changes_callback(std::move(apply_changes_callback)),
      cancel_callback(std::move(cancel_callback)),
      never_ask_me_callback(std::move(never_ask_me_callback)) {}

PsstDialogDelegate::ShowDialogData::~ShowDialogData() = default;

void PsstDialogDelegate::AddObserver(Observer* obs) {
  observer_list_.AddObserver(obs);
}
void PsstDialogDelegate::RemoveObserver(Observer* obs) {
  observer_list_.RemoveObserver(obs);
}
bool PsstDialogDelegate::HasObserver(Observer* observer) {
  return observer_list_.HasObserver(observer);
}

void PsstDialogDelegate::ShowPsstConsentDialog(
    content::WebContents* contents,
    std::unique_ptr<ShowDialogData> show_dialog_data) {
  show_dialog_data_ = std::move(show_dialog_data);
}

PsstDialogDelegate::ShowDialogData* PsstDialogDelegate::GetShowDialogData() {
  return show_dialog_data_ ? show_dialog_data_.get() : nullptr;
}

void PsstDialogDelegate::SetRequestDone(
    content::WebContents* contents,
    const std::string& url,
    const std::optional<std::string>& error) {
  for (Observer& obs : observer_list_) {
    obs.OnSetRequestDone(url, error);
  }
}

void PsstDialogDelegate::SetCompletedView(
    content::WebContents* contents,
    const std::vector<std::string>& applied_checks,
    const std::vector<std::string>& errors,
    ShareCallback share_cb) {
  for (Observer& obs : observer_list_) {
    obs.OnSetCompleted(applied_checks, errors);
  }
}

}  // namespace psst
