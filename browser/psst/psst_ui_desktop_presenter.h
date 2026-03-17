// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_UI_DESKTOP_PRESENTER_H_
#define BRAVE_BROWSER_PSST_PSST_UI_DESKTOP_PRESENTER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/psst/psst_infobar_delegate.h"
#include "brave/browser/psst/psst_ui_presenter.h"
#include "content/public/browser/web_contents.h"

namespace psst {

class UiDesktopPresenter : public PsstUiPresenter {
 public:
  explicit UiDesktopPresenter(content::WebContents* web_contents);
  ~UiDesktopPresenter() override;

  void ShowInfoBar(
      PsstInfoBarDelegate::AcceptCallback on_accept_callback) override;

 private:
  void OnInfobarAccepted(PsstInfoBarDelegate::AcceptCallback on_accept_callback,
                         const bool is_accepted);

  raw_ptr<content::WebContents> web_contents_;
  base::WeakPtrFactory<UiDesktopPresenter> weak_ptr_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_UI_DESKTOP_PRESENTER_H_
