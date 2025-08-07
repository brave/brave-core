// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_ui_delegate.h"

namespace psst {

PsstUiDelegate::PsstUiDelegate() = default;
PsstUiDelegate::~PsstUiDelegate() = default;

PsstUiDelegate::ShowDialogData::ShowDialogData(
    const std::string& user_id,
    const std::string& site_name,
    base::Value::List request_infos,
    const int script_version,
    ConsentCallback apply_changes_callback)
    : user_id(user_id),
      site_name(site_name),
      request_infos(std::move(request_infos)),
      script_version(script_version),
      apply_changes_callback(std::move(apply_changes_callback)) {}
PsstUiDelegate::ShowDialogData::~ShowDialogData() = default;

PsstUiDelegate::ShowDialogData::ShowDialogData(ShowDialogData&&) noexcept =
    default;
PsstUiDelegate::ShowDialogData& PsstUiDelegate::ShowDialogData::operator=(
    ShowDialogData&&) noexcept = default;

}  // namespace psst
