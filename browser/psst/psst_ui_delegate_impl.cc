// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/psst/psst_ui_delegate_impl.h"

#include "brave/browser/psst/brave_psst_permission_context_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"

namespace {
std::vector<std::string> ListToVector(const base::Value::List list) {
  std::vector<std::string> result;
  result.reserve(list.size());
  for (const auto& v : list) {
    CHECK(v.is_string());
    result.push_back(v.GetString());
  }
  return result;
}
}  // namespace

namespace psst {

PsstUiDelegateImpl::PsstUiDelegateImpl(Profile* profile,
                                       content::WebContents* contents)
    : web_contents_(contents), profile_(profile) {
  CHECK(profile);
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
  auto* psst_permission_context =
      BravePsstPermissionContextFactory::GetForBrowserContext(profile_);
  if (!psst_permission_context) {
    return std::nullopt;
  }
  return psst_permission_context->GetPsstPermissionInfo(origin, user_id);
}

void PsstUiDelegateImpl::OnUserAcceptedPsstSettings(
    base::Value::List urls_to_skip) {
  auto* psst_permission_context =
      BravePsstPermissionContextFactory::GetForBrowserContext(profile_);
  CHECK(psst_permission_context);

  // Create the PSST permission when user accepts the dialog
  psst_permission_context->CreateOrUpdate(
      url::Origin::Create(web_contents_->GetLastCommittedURL()),
      ConsentStatus::kAllow, dialog_data_->script_version,
      dialog_data_->user_id, urls_to_skip.Clone());

  if (dialog_data_->apply_changes_callback) {
    std::move(dialog_data_->apply_changes_callback)
        .Run(ListToVector(std::move(urls_to_skip)));
  }
}

}  // namespace psst
