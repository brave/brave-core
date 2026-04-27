// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_UI_DESKTOP_PRESENTER_H_
#define BRAVE_BROWSER_PSST_PSST_UI_DESKTOP_PRESENTER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/psst/psst_ui_presenter.h"
#include "chrome/browser/ui/dialogs/browser_dialogs.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

namespace content {
class WebContents;
}  // namespace content

class ConstrainedWebDialogDelegate;

namespace psst {

// Implementation of PsstUiPresenter for desktop platforms
class PsstUiDesktopPresenter : public PsstUiPresenter {
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
      base::WeakPtr<content::WebContents> web_contents);
  ~PsstUiDesktopPresenter() override;

  void ShowInfoBar(InfoBarCallback on_accept_callback) override;

  void ShowConsentDialog() override;

  bool IsDialogShown() const override;

 private:
  base::WeakPtr<content::WebContents> web_contents_;
  base::WeakPtr<psst::PsstUiDesktopPresenter::PsstUiDesktopDelegate>
      dialog_delegate_;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_UI_DESKTOP_PRESENTER_H_
