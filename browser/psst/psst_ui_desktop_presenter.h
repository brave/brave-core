// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_UI_DESKTOP_PRESENTER_H_
#define BRAVE_BROWSER_PSST_PSST_UI_DESKTOP_PRESENTER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/psst/psst_ui_presenter.h"
#include "brave/browser/ui/views/page_action/psst_action_controller.h"
#include "chrome/browser/ui/dialogs/browser_dialogs.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

namespace content {
class WebContents;
}  // namespace content

class ConstrainedWebDialogDelegate;

namespace psst {

// Implementation of PsstUiPresenter for desktop platforms
class PsstUiDesktopPresenter
    : public PsstUiPresenter,
      public page_actions::PsstActionController::Observer {
 public:
  class PsstUiDesktopDelegate : public ui::WebDialogDelegate {
   public:
    static PsstUiDesktopDelegate* GetDelegateFromWebContents(
        content::WebContents* web_contents);
    explicit PsstUiDesktopDelegate(
        base::WeakPtr<content::WebContents> initiator_web_contents);
    PsstUiDesktopDelegate(const PsstUiDesktopDelegate&) = delete;
    PsstUiDesktopDelegate& operator=(const PsstUiDesktopDelegate&) = delete;
    ~PsstUiDesktopDelegate() override;

    void SetDelegateToWebContents(
        ConstrainedWebDialogDelegate* dialog_delegate);
    content::WebContents* GetInitiatorWebContents() const;

    // ui::WebDialogDelegate:
    void OnDialogClosed(const std::string& json_retval) override;

    base::WeakPtr<PsstUiDesktopDelegate> GetWeakPtr();
    bool IsDialogShown() const;

    void CloseDialog();

   private:
    base::WeakPtr<content::WebContents> initiator_web_contents_;  // unowned
    raw_ptr<ConstrainedWebDialogDelegate> web_dialog_delegate_ =
        nullptr;  // unowned
    base::WeakPtrFactory<PsstUiDesktopDelegate> weak_ptr_factory_{this};
  };

  explicit PsstUiDesktopPresenter(
      base::WeakPtr<content::WebContents> web_contents,
      base::WeakPtr<page_actions::PsstActionController> psst_action_controller);
  ~PsstUiDesktopPresenter() override;

  void SetLocationBarIconStatus(
      LocationBarIconStatus status,
      LocationBarMenuCallback on_dont_show_this_site_callback,
      LocationBarMenuCallback on_disable_privacy_settings_tuning_callback)
      override;

  void ShowInfoBar(InfoBarCallback on_accept_callback) override;
  void HideInfoBar() override;

  void ShowConsentDialog() override;

  bool IsDialogShown() const override;

 private:
  // page_actions::PsstActionController::Observer:
  void OnDontShowThisSiteSelected() override;
  void OnDisablePrivacySettingsTuningSelected() override;

  base::WeakPtr<content::WebContents> web_contents_;
  base::WeakPtr<page_actions::PsstActionController> psst_action_controller_;
  base::WeakPtr<psst::PsstUiDesktopPresenter::PsstUiDesktopDelegate>
      dialog_delegate_;
  LocationBarMenuCallback on_dont_show_this_site_callback_;
  LocationBarMenuCallback on_disable_privacy_settings_tuning_callback_;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_UI_DESKTOP_PRESENTER_H_
