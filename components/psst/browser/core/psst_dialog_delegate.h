/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_DIALOG_DELEGATE_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_DIALOG_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/functional/callback.h"

namespace psst {

// Represents the PSST dialog delegate interface.
// Allows to manage by the PSST dialog's lifecycle and provide data to it.
class COMPONENT_EXPORT(PSST_BROWSER_CORE) PsstDialogDelegate {
 public:
  using ConsentCallback =
      base::OnceCallback<void(const std::vector<std::string>& disabled_checks)>;
  using ShareCallback = base::OnceCallback<void()>;

  // Data structure to hold information, required for displaying the PSST
  // dialog.
  struct COMPONENT_EXPORT(PSST_BROWSER_CORE) ShowDialogData {};

  PsstDialogDelegate();
  virtual ~PsstDialogDelegate();
  virtual void SetProgressValue(const double value);
  virtual void SetRequestDone(const std::string& url,
                              const std::optional<std::string>& error);
  virtual void SetCompletedView(
      const std::optional<std::vector<std::string>>& applied_checks,
      const std::optional<std::vector<std::string>>& errors);
  virtual void Show();
  virtual void Close();

  void SetShowDialogData(std::unique_ptr<ShowDialogData> show_dialog_data);
  ShowDialogData* GetShowDialogData();

 private:
  std::unique_ptr<ShowDialogData> show_dialog_data_;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_DIALOG_DELEGATE_H_
