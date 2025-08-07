// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_ui_delegate_impl.h"

#include "brave/browser/psst/psst_ui_presenter.h"
#include "brave/components/psst/common/pref_names.h"
#include "brave/components/psst/common/psst_common.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"

namespace psst {

PsstUiDelegateImpl::PsstUiDelegateImpl(
    Profile* profile,
    content::WebContents* contents,
    std::unique_ptr<PsstUiPresenter> ui_presenter)
    : web_contents_(contents),
      prefs_(profile->GetPrefs()),
      ui_presenter_(std::move(ui_presenter)) {
  CHECK(profile);
  CHECK(web_contents_);

  auto* map = HostContentSettingsMapFactory::GetForProfile(profile);
  CHECK(map);
  psst_permission_context_ = std::make_unique<BravePsstPermissionContext>(map);
}

PsstUiDelegateImpl::~PsstUiDelegateImpl() = default;

void PsstUiDelegateImpl::ShowPsstInfobar(InfobarCallback callback) {
  if (!prefs_->GetBoolean(prefs::kShowPsstInfoBar)) {
    // If user has already interacted with infobar
    std::move(callback).Run(true);
    return;
  }
  ui_presenter_->ShowInfoBar(
      base::BindOnce(&PsstUiDelegateImpl::OnInfobarAccepted,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PsstUiDelegateImpl::Show(ShowDialogData show_dialog_data) {
  url::Origin origin =
      url::Origin::Create(web_contents_->GetLastCommittedURL());
  psst_permission_info_ = psst_permission_context_->GetPsstPermissionInfo(
      origin, show_dialog_data.user_id);

  if (psst_permission_info_ &&
      psst_permission_info_->consent_status == psst::ConsentStatus::kBlock) {
    return;  // Do nothing if the user has blocked the PSST for that site
  }

  ui_presenter_->ShowIcon();

  show_dialog_data_ = std::move(show_dialog_data);

  // When dialog accepted by the user
  OnUserAcceptedPsstSettings(base::Value::List());
}

void PsstUiDelegateImpl::SetProgress(const double value) {}

void PsstUiDelegateImpl::Close() {}

void PsstUiDelegateImpl::SetCompleted() {}

std::optional<PsstPermissionInfo> PsstUiDelegateImpl::GetPsstPermissionInfo(
    const url::Origin& origin,
    const std::string& user_id) {
  if (!psst_permission_context_) {
    return std::nullopt;
  }
  return psst_permission_context_->GetPsstPermissionInfo(origin, user_id);
}

void PsstUiDelegateImpl::OnInfobarAccepted(InfobarCallback callback,
                                           const bool is_accepted) {
  prefs_->SetBoolean(prefs::kShowPsstInfoBar, false);

  if (!callback) {
    return;
  }

  std::move(callback).Run(is_accepted);
}

void PsstUiDelegateImpl::OnUserAcceptedPsstSettings(
    base::Value::List urls_to_skip) {
  // Create the PSST permission when user accepts the dialog
  psst_permission_context_->CreateOrUpdate(
      url::Origin::Create(web_contents_->GetLastCommittedURL()),
      PsstPermissionInfo{ConsentStatus::kAllow,
                         show_dialog_data_->script_version,
                         show_dialog_data_->user_id, urls_to_skip.Clone()});

  if (show_dialog_data_->apply_changes_callback) {
    std::move(show_dialog_data_->apply_changes_callback)
        .Run(std::move(urls_to_skip));
  }
}

}  // namespace psst
