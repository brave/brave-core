// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_UI_DELEGATE_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_UI_DELEGATE_H_

#include <string>

#include "base/functional/callback.h"
#include "base/values.h"
#include "brave/components/psst/common/psst_common.h"
#include "url/origin.h"

namespace psst {

class PsstUiDelegate {
 public:
  PsstUiDelegate();
  virtual ~PsstUiDelegate();

  PsstUiDelegate(const PsstUiDelegate&) = delete;
  PsstUiDelegate& operator=(const PsstUiDelegate&) = delete;

  using InfobarCallback = base::OnceCallback<void(const bool is_accepted)>;
  using ConsentCallback = base::OnceCallback<void(
      std::optional<base::Value::List> disabled_checks)>;

  struct ShowDialogData {
    ShowDialogData(const std::string& user_id,
                   const std::string& site_name,
                   base::Value::List request_infos,
                   const int script_version,
                   ConsentCallback apply_changes_callback);
    ~ShowDialogData();

    ShowDialogData(ShowDialogData&&) noexcept;
    ShowDialogData& operator=(ShowDialogData&&) noexcept;

    ShowDialogData(const ShowDialogData&) = delete;
    ShowDialogData& operator=(const ShowDialogData&) = delete;

    std::string user_id;
    std::string site_name;
    base::Value::List request_infos;
    int script_version;
    ConsentCallback apply_changes_callback;
  };

  virtual void SetProgress(const double value) = 0;
  virtual void SetCompleted() = 0;
  virtual void ShowPsstInfobar(InfobarCallback callback) = 0;
  virtual void Show(ShowDialogData show_dialog_data) = 0;
  virtual void Close() = 0;

  virtual std::optional<PsstPermissionInfo> GetPsstPermissionInfo(
      const url::Origin& origin,
      const std::string& user_id) = 0;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_UI_DELEGATE_H_
