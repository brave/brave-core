/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/browser/core/psst_dialog_delegate.h"

#include <memory>
#include <utility>

namespace psst {

PsstDialogDelegate::PsstDialogDelegate() = default;
PsstDialogDelegate::~PsstDialogDelegate() = default;

void PsstDialogDelegate::SetProgressValue(const double value) {
  // Default implementation does nothing.
}

PsstDialogDelegate::ShowDialogData* PsstDialogDelegate::GetShowDialogData() {
  return show_dialog_data_ ? show_dialog_data_.get() : nullptr;
}

void PsstDialogDelegate::SetRequestDone(
    const std::string& url,
    const std::optional<std::string>& error) {
  // Default implementation does nothing.
}

void PsstDialogDelegate::SetCompletedView(
    const std::optional<std::vector<std::string>>& applied_checks,
    const std::optional<std::vector<std::string>>& errors) {
  // Default implementation does nothing.
}

void PsstDialogDelegate::SetShowDialogData(
    std::unique_ptr<ShowDialogData> show_dialog_data) {
  show_dialog_data_ = std::move(show_dialog_data);
}

void PsstDialogDelegate::Show() {
  // Default implementation does nothing.
  // Derived classes should implement this method to show the dialog.
}

void PsstDialogDelegate::Close() {
  // Default implementation does nothing.
  // Derived classes should implement this method to close the dialog.
}

}  // namespace psst
