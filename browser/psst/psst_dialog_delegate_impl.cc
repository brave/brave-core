// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_dialog_delegate_impl.h"

#include <optional>

#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
namespace psst {

PsstDialogDelegateImpl::PsstDialogDelegateImpl(content::WebContents* contents)
    : web_contents_(contents) {
  Profile* profile = Profile::FromBrowserContext(contents->GetBrowserContext());
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile);
  CHECK(map);
  psst_permission_context_ = std::make_unique<BravePsstPermissionContext>(map);
}

PsstDialogDelegateImpl::~PsstDialogDelegateImpl() = default;

void PsstDialogDelegateImpl::Show(
    PsstTabWebContentsObserver::ShowDialogData show_dialog_data) {
  // Open Psst dialog

  // Simulate when dialog accepted by the user
  // Grant permission
  psst_permission_context_->CreateOrUpdate(
      url::Origin::Create(web_contents_->GetLastCommittedURL()),
      PsstPermissionInfo{ConsentStatus::kAllow, show_dialog_data.script_version,
                         show_dialog_data.user_id, base::Value::List()});
  if (show_dialog_data.apply_changes_callback) {
    std::move(show_dialog_data.apply_changes_callback)
        .Run(ConsentStatus::kAllow, std::nullopt);
  }
}

std::optional<PsstPermissionInfo> PsstDialogDelegateImpl::GetPsstPermissionInfo(
    const url::Origin& origin,
    const std::string& user_id) {
  if (!psst_permission_context_) {
    return std::nullopt;
  }
  return psst_permission_context_->GetPsstPermissionInfo(origin, user_id);
}

}  // namespace psst
