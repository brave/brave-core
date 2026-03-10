// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_UI_PRESENTER_H_
#define BRAVE_BROWSER_PSST_PSST_UI_PRESENTER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/psst/psst_infobar_delegate.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

namespace content {
class WebContents;
}  // namespace content

namespace psst {

class PsstUiPresenter {
 public:
  virtual ~PsstUiPresenter() = default;

  virtual void ShowInfoBar(
      PsstInfoBarDelegate::AcceptCallback on_accept_callback) = 0;
  virtual void ShowIcon() = 0;
};

class UiDesktopPresenter : public PsstUiPresenter {
 public:
  class UiDesktopDelegate : public ui::WebDialogDelegate {
   public:
    void SetDelegateToWebContents(content::WebContents* dialog_web_contents);
    static UiDesktopDelegate* GetDelegateFromWebContents(
        content::WebContents* web_contents);

    explicit UiDesktopDelegate(base::WeakPtr<content::WebContents> initiator_web_contents);
    UiDesktopDelegate(const UiDesktopDelegate&) = delete;
    UiDesktopDelegate& operator=(const UiDesktopDelegate&) = delete;
    ~UiDesktopDelegate() override;

    content::WebContents* GetInitiatorWebContents() const;

    // ui::WebDialogDelegate:
    void OnDialogClosed(const std::string& json_retval) override;

   private:
    base::WeakPtr<content::WebContents> initiator_web_contents_;  // unowned
    raw_ptr<content::WebContents> dialog_web_contents_;     // unowned
  };

  explicit UiDesktopPresenter(content::WebContents* web_contents);
  ~UiDesktopPresenter() override;

  void ShowInfoBar(
      PsstInfoBarDelegate::AcceptCallback on_accept_callback) override;
  void ShowIcon() override;

 private:
  void OnInfobarAccepted(PsstInfoBarDelegate::AcceptCallback on_accept_callback,
                         const bool is_accepted);

  raw_ptr<content::WebContents> web_contents_;
  base::WeakPtrFactory<UiDesktopPresenter> weak_ptr_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_UI_PRESENTER_H_
