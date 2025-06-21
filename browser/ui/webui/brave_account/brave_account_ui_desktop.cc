/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_account/brave_account_ui_desktop.h"

#include <memory>

#include "base/check.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "content/public/browser/web_ui.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/widget/widget.h"
#include "ui/web_dialogs/web_dialog_delegate.h"
#include "url/gurl.h"

namespace {

constexpr float kDialogBorderRadius = 16;
constexpr int kDialogWidth = 500;
constexpr gfx::Size kDialogMinSize(kDialogWidth, 470);
constexpr gfx::Size kDialogMaxSize(kDialogWidth, 794);

class BraveAccountDialogDelegate : public ui::WebDialogDelegate {
 public:
  BraveAccountDialogDelegate() {
    set_delete_on_close(false);
    set_dialog_content_url(GURL(kBraveAccountURL));
    set_show_dialog_title(false);
  }
};

}  // namespace

void ShowBraveAccountDialog(content::WebUI* web_ui) {
  DCHECK(web_ui);
  auto* delegate = ShowConstrainedWebDialogWithAutoResize(
      Profile::FromWebUI(web_ui),
      std::make_unique<BraveAccountDialogDelegate>(), web_ui->GetWebContents(),
      kDialogMinSize, kDialogMaxSize);

  DCHECK(delegate);
  auto* widget =
      views::Widget::GetWidgetForNativeWindow(delegate->GetNativeDialog());
  if (!widget) {
    return;
  }

  if (auto* layer = widget->GetLayer()) {
    layer->SetRoundedCornerRadius(gfx::RoundedCornersF(kDialogBorderRadius));
  }
}
