// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_UI_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_PSST_PSST_UI_DELEGATE_IMPL_H_

#include <optional>

#include "base/memory/raw_ptr.h"
#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"
#include "brave/components/psst/browser/core/brave_psst_permission_context.h"
#include "brave/components/psst/browser/core/psst_consent_data.h"
#include "brave/components/psst/common/psst_permission_schema.h"
#include "brave/components/psst/common/psst_script_responses.h"
#include "brave/components/psst/common/psst_ui_common.mojom-shared.h"

namespace content {
class WebContents;
}  // namespace content

namespace psst {

class PsstUiPresenter;

class PsstUiDelegateImpl : public PsstTabWebContentsObserver::PsstUiDelegate {
 public:
  explicit PsstUiDelegateImpl(BravePsstPermissionContext* permission_context, std::unique_ptr<PsstUiPresenter> ui_presenter);
  ~PsstUiDelegateImpl() override;

  PsstUiDelegateImpl(const PsstUiDelegateImpl&) = delete;
  PsstUiDelegateImpl& operator=(const PsstUiDelegateImpl&) = delete;

  void Show(PsstConsentData dialog_data, permissions::PermissionPrompt::Delegate* delegate) override;

  void ShowPsstInfobar(PsstTabWebContentsObserver::InfobarCallback callback, permissions::PermissionPrompt::Delegate* delegate, PsstConsentData dialog_data) override;

  // PsstUiDelegate overrides
  void UpdateTasks(long progress,
                   const std::vector<PolicyTask>& applied_tasks,
                   const mojom::PsstStatus status) override;

  std::optional<PsstPermissionInfo> GetPsstPermissionInfo(
      const url::Origin& origin,
      const std::string& user_id) override;

 private:
  void OnInfobarAccepted(PsstTabWebContentsObserver::InfobarCallback callback, const bool is_accepted);
  void OnUserAcceptedPsstSettings(base::Value::List urls_to_skip);

  raw_ptr<content::WebContents> web_contents_;
  raw_ptr<BravePsstPermissionContext> permission_context_;
  std::unique_ptr<PsstUiPresenter> ui_presenter_;
  raw_ptr<permissions::PermissionPrompt::Delegate> delegate_{nullptr};
  std::optional<PsstConsentData> dialog_data_;
  base::WeakPtrFactory<PsstUiDelegateImpl> weak_ptr_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_UI_DELEGATE_IMPL_H_
