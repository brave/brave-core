// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_DIALOG_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_PSST_PSST_DIALOG_DELEGATE_IMPL_H_

#include "base/memory/raw_ptr.h"
#include "brave/browser/psst/brave_psst_permission_context.h"
#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"

namespace content {
class WebContents;
}  // namespace content

namespace psst {

class PsstDialogDelegateImpl
    : public PsstTabWebContentsObserver::PsstDialogDelegate {
 public:
  explicit PsstDialogDelegateImpl(content::WebContents* contents);
  ~PsstDialogDelegateImpl() override;

  void Show(
      PsstTabWebContentsObserver::ShowDialogData show_dialog_data) override;

  std::optional<PsstPermissionInfo> GetPsstPermissionInfo(
      const url::Origin& origin,
      const std::string& user_id) override;

 private:
  raw_ptr<content::WebContents> web_contents_;
  std::unique_ptr<BravePsstPermissionContext> psst_permission_context_;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_DIALOG_DELEGATE_IMPL_H_
