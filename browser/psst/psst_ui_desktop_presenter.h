// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_UI_DESKTOP_PRESENTER_H_
#define BRAVE_BROWSER_PSST_PSST_UI_DESKTOP_PRESENTER_H_

#include "base/memory/weak_ptr.h"
#include "brave/browser/psst/psst_ui_presenter.h"
#include "chrome/browser/ui/dialogs/browser_dialogs.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

namespace content {
class WebContents;
}  // namespace content

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
        base::WeakPtr<content::WebContents> dialog_web_contents);
    content::WebContents* GetInitiatorWebContents() const;

    // ui::WebDialogDelegate:
    void OnDialogClosed(const std::string& json_retval) override;

   private:
    base::WeakPtr<content::WebContents> initiator_web_contents_;  // unowned
    base::WeakPtr<content::WebContents> dialog_web_contents_;     // unowned
  };

  explicit PsstUiDesktopPresenter(
      base::WeakPtr<content::WebContents> web_contents);
  ~PsstUiDesktopPresenter() override;

  void ShowInfoBar(InfoBarCallback on_accept_callback) override;

  void ShowConsentDialog() override;

 private:
  base::WeakPtr<content::WebContents> web_contents_;
  base::WeakPtrFactory<PsstUiDesktopPresenter> weak_ptr_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_UI_DESKTOP_PRESENTER_H_
