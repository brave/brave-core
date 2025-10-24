// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PSST_PSST_UI_PRESENTER_H_
#define BRAVE_BROWSER_PSST_PSST_UI_PRESENTER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/psst/brave_psst_infobar_delegate.h"

namespace content {
class WebContents;
}  // namespace content

namespace psst {

class PsstUiPresenter {
 public:
  virtual ~PsstUiPresenter() = default;

  virtual void ShowInfoBar(
      BravePsstInfoBarDelegate::AcceptCallback on_accept_callback) = 0;
  virtual void ShowDialog() = 0;
  virtual void ShowIcon() = 0;
};

class UiDesktopPresenter : public PsstUiPresenter {
 public:
  explicit UiDesktopPresenter(content::WebContents* web_contents);
  ~UiDesktopPresenter() override;

  void ShowInfoBar(
      BravePsstInfoBarDelegate::AcceptCallback on_accept_callback) override;
  void ShowIcon() override;
  void ShowDialog() override;

 private:
  void OnInfobarAccepted(
      BravePsstInfoBarDelegate::AcceptCallback on_accept_callback,
      const bool is_accepted);

  raw_ptr<content::WebContents> web_contents_;
  base::WeakPtrFactory<UiDesktopPresenter> weak_ptr_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_BROWSER_PSST_PSST_UI_PRESENTER_H_
