/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_account/brave_account_ui_desktop.h"

#include <memory>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/url_constants.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/widget/widget.h"
#include "ui/web_dialogs/web_dialog_delegate.h"
#include "ui/webui/webui_util.h"
#include "url/gurl.h"

namespace {

constexpr float kDialogBorderRadius = 16;
constexpr int kDialogWidth = 500;
constexpr gfx::Size kDialogMinSize(kDialogWidth, 336);
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

BraveAccountUIDesktop::BraveAccountUIDesktop(content::WebUI* web_ui)
    : BraveAccountUIBase(Profile::FromWebUI(web_ui),
                         base::BindOnce(&webui::SetupWebUIDataSource)),
      ConstrainedWebDialogUI(web_ui) {}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveAccountUIDesktop)

BraveAccountUIDesktopConfig::BraveAccountUIDesktopConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kBraveAccountHost) {
  CHECK(brave_account::features::IsBraveAccountEnabled());
}

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
