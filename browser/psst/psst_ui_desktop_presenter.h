// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_UI_DESKTOP_PRESENTER_H_
#define BRAVE_BROWSER_PSST_PSST_UI_DESKTOP_PRESENTER_H_

#include "base/memory/raw_ptr.h"
#include "brave/browser/psst/psst_ui_presenter.h"
#include "content/public/browser/web_contents.h"

namespace psst {

// Implementation of PsstUiPresenter for desktop platforms
class UiDesktopPresenter : public PsstUiPresenter {
 public:
  explicit UiDesktopPresenter(content::WebContents* web_contents);
  ~UiDesktopPresenter() override;

  void ShowInfoBar(InfoBarCallback on_accept_callback) override;

 private:
  raw_ptr<content::WebContents> web_contents_;
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_UI_DESKTOP_PRESENTER_H_
