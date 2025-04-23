/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/psst/brave_psst_dialog.h"

#include <memory>
#include <string>

#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

namespace psst {

namespace {

constexpr int kDialogMinHeight = 100;
constexpr int kDialogMaxHeight = 700;
constexpr int kDialogWidth = 475;

}  // namespace
class PsstWebDialogDelegate : public ui::WebDialogDelegate {
 public:
  PsstWebDialogDelegate();
  PsstWebDialogDelegate(const PsstWebDialogDelegate&) = delete;
  PsstWebDialogDelegate& operator=(const PsstWebDialogDelegate&) = delete;
  ~PsstWebDialogDelegate() override;

  GURL GetDialogContentURL() const override;
  void OnDialogClosed(const std::string& json_retval) override;
  void OnCloseContents(content::WebContents* source,
                       bool* out_close_dialog) override;
};

PsstWebDialogDelegate::PsstWebDialogDelegate() {
  set_show_dialog_title(false);
}

PsstWebDialogDelegate::~PsstWebDialogDelegate() = default;

GURL PsstWebDialogDelegate::GetDialogContentURL() const {
  return GURL(kBraveUIPsstURL);
}
void PsstWebDialogDelegate::OnDialogClosed(
    const std::string& /* json_retval */) {}

void PsstWebDialogDelegate::OnCloseContents(content::WebContents* /* source */,
                                            bool* out_close_dialog) {
  *out_close_dialog = true;
}

void OpenPsstDialog(content::WebContents* initiator) {
  gfx::Size min_size(kDialogWidth, kDialogMinHeight);
  gfx::Size max_size(kDialogWidth, kDialogMaxHeight);
  ShowConstrainedWebDialogWithAutoResize(
      initiator->GetBrowserContext(), std::make_unique<PsstWebDialogDelegate>(),
      initiator, min_size, max_size);
}

}  // namespace psst
