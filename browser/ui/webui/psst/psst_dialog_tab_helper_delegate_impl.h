/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_PSST_PSST_DIALOG_TAB_HELPER_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_UI_WEBUI_PSST_PSST_DIALOG_TAB_HELPER_DELEGATE_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/psst/browser/core/psst_dialog_delegate.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"

namespace psst {
class PsstDialogTabHelperDelegateImpl
    : public PsstDialogDelegate {
 public:
  PsstDialogTabHelperDelegateImpl();
  ~PsstDialogTabHelperDelegateImpl() override;

  // psst::PsstTabHelper::Delegate
  void ShowPsstConsentDialog(
      content::WebContents* contents,
      std::unique_ptr<ShowDialogData> show_dialog_data) override;
  void SetProgressValue(content::WebContents* contents,
                        const double value) override;
  void SetRequestDone(content::WebContents* contents,
                      const std::string& url,
                      const std::optional<std::string>& error) override;
  void SetCompletedView(content::WebContents* contents,
                        const std::vector<std::string>& applied_checks,
                        const std::vector<std::string>& errors,
                        ShareCallback share_cb) override;
  void Close(content::WebContents* contents) override;

 private:
  //  void AddObserver(Observer* obs) override;

  //  raw_ptr<ConstrainedWebDialogDelegate>
  //  constrained_webdlg_delegate_{nullptr};
};

}  // namespace psst

#endif  // BRAVE_BROWSER_UI_WEBUI_PSST_PSST_DIALOG_TAB_HELPER_DELEGATE_IMPL_H_
