// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_UI_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_PSST_PSST_UI_DELEGATE_IMPL_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/browser/psst/brave_psst_permission_context.h"
#include "brave/components/psst/browser/content/psst_ui_delegate.h"
#include "url/origin.h"

namespace content {
class WebContents;
}  // namespace content

class Profile;
class PrefService;

namespace psst {

class PsstUiPresenter;
class BravePsstUIDelegateImplUnitTest;

class PsstUiDelegateImpl : public PsstUiDelegate {
 public:
  explicit PsstUiDelegateImpl(Profile* profile,
                              content::WebContents* contents,
                              std::unique_ptr<PsstUiPresenter> ui_presenter);
  ~PsstUiDelegateImpl() override;

  void SetProgress(const double value) override;
  void SetCompleted() override;

  void ShowPsstInfobar(InfobarCallback callback) override;
  void Show(ShowDialogData show_dialog_data) override;
  void Close() override;

  std::optional<PsstPermissionInfo> GetPsstPermissionInfo(
      const url::Origin& origin,
      const std::string& user_id) override;

 private:
  void OnInfobarAccepted(InfobarCallback callback, const bool is_accepted);
  void OnUserAcceptedPsstSettings(base::Value::List urls_to_skip);

  raw_ptr<content::WebContents> web_contents_;
  std::unique_ptr<BravePsstPermissionContext> psst_permission_context_;
  raw_ptr<PrefService> prefs_;
  std::unique_ptr<PsstUiPresenter> ui_presenter_;
  std::optional<ShowDialogData> show_dialog_data_;
  std::optional<PsstPermissionInfo> psst_permission_info_;
  friend class BravePsstUIDelegateImplUnitTest;
  base::WeakPtrFactory<PsstUiDelegateImpl> weak_ptr_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_UI_DELEGATE_IMPL_H_
