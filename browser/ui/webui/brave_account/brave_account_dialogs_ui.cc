/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_account/brave_account_dialogs_ui.h"

#include "base/containers/span.h"
#include "brave/components/brave_account/resources/grit/brave_account_resources.h"
#include "brave/components/brave_account/resources/grit/brave_account_resources_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "ui/compositor/layer.h"
#include "ui/views/widget/widget.h"
#include "ui/webui/webui_util.h"

BraveAccountDialogsUI::BraveAccountDialogsUI(content::WebUI* web_ui)
    : ConstrainedWebDialogUI(web_ui),
      BraveAccountDialogsUIBase(Profile::FromWebUI(web_ui)) {}

BraveAccountDialogsUI::~BraveAccountDialogsUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(BraveAccountDialogsUI)

BraveAccountDialogsDialog::BraveAccountDialogsDialog() = default;

void BraveAccountDialogsDialog::Show(content::WebUI* web_ui) {
  // chrome::ShowWebDialog(web_ui->GetWebContents()->GetNativeView(),
  //                       Profile::FromWebUI(web_ui), new
  //                       BraveAccountDialogsDialog());
  const int kSigninEmailConfirmationDialogWidth = 500;
  const int kSigninEmailConfirmationDialogMinHeight = 470;
  const int kSigninEmailConfirmationDialogMaxHeight = 794;
  gfx::Size min_size(kSigninEmailConfirmationDialogWidth,
                     kSigninEmailConfirmationDialogMinHeight);
  gfx::Size max_size(kSigninEmailConfirmationDialogWidth,
                     kSigninEmailConfirmationDialogMaxHeight);

  auto* delegate = ShowConstrainedWebDialogWithAutoResize(
      Profile::FromWebUI(web_ui),
      base::WrapUnique(new BraveAccountDialogsDialog()),
      web_ui->GetWebContents(), min_size, max_size);
  DCHECK(delegate);
  auto* widget =
      views::Widget::GetWidgetForNativeWindow(delegate->GetNativeDialog());
  if (widget && widget->GetLayer()) {
    widget->GetLayer()->SetRoundedCornerRadius(gfx::RoundedCornersF(16));
  }
}

ui::mojom::ModalType BraveAccountDialogsDialog::GetDialogModalType() const {
  return ui::mojom::ModalType::kWindow;
}

std::u16string BraveAccountDialogsDialog::GetDialogTitle() const {
  return u"";
}

GURL BraveAccountDialogsDialog::GetDialogContentURL() const {
  return GURL(kBraveAccountDialogsURL);
}

void BraveAccountDialogsDialog::GetWebUIMessageHandlers(
    std::vector<content::WebUIMessageHandler*>* handlers) {}

void BraveAccountDialogsDialog::GetDialogSize(gfx::Size* size) const {
  const int kDefaultWidth = 500;
  const int kDefaultHeight = 754;
  size->SetSize(kDefaultWidth, kDefaultHeight);
}

std::string BraveAccountDialogsDialog::GetDialogArgs() const {
  return "";
}

void BraveAccountDialogsDialog::OnDialogShown(content::WebUI* webui) {
  webui_ = webui;
}

void BraveAccountDialogsDialog::OnDialogClosed(const std::string& json_retval) {
  VLOG(0) << "deleted";
  // delete this;
}

void BraveAccountDialogsDialog::OnCloseContents(content::WebContents* source,
                                                bool* out_close_dialog) {
  *out_close_dialog = true;
}

bool BraveAccountDialogsDialog::ShouldShowDialogTitle() const {
  return false;
}

BraveAccountDialogsDialog::~BraveAccountDialogsDialog() {
  VLOG(0) << "~BraveAccountDialogsDialog()";
}
