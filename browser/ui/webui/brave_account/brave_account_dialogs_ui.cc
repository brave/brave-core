/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_account/brave_account_dialogs_ui.h"

#include <memory>

#include "base/check.h"
#include "brave/browser/brave_account/brave_account_service_factory.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/url_constants.h"
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

class BraveAccountDialogs : public ui::WebDialogDelegate {
 public:
  BraveAccountDialogs() {
    set_delete_on_close(false);
    set_dialog_content_url(GURL(kBraveAccountDialogsURL));
    set_show_dialog_title(false);
  }
};

}  // namespace

BraveAccountDialogsUI::BraveAccountDialogsUI(content::WebUI* web_ui)
    : ConstrainedWebDialogUI(web_ui),
      BraveAccountDialogsUIBase(Profile::FromWebUI(web_ui)) {
  brave_account::BraveAccountServiceFactory::GetForBrowserContext(
      Profile::FromWebUI(web_ui));
}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveAccountDialogsUI)

BraveAccountDialogsUIConfig::BraveAccountDialogsUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kBraveAccountDialogsHost) {}

bool BraveAccountDialogsUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return true;
}

void ShowBraveAccountDialogs(content::WebUI* web_ui) {
  auto* delegate = ShowConstrainedWebDialogWithAutoResize(
      Profile::FromWebUI(web_ui), std::make_unique<BraveAccountDialogs>(),
      web_ui->GetWebContents(), kDialogMinSize, kDialogMaxSize);

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
