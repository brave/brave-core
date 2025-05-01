/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/hello_world/hello_world_ui.h"

#include "base/containers/span.h"
#include "base/version_info/version_info.h"
#include "brave/components/hello_world/resources/grit/hello_world_resources.h"
#include "brave/components/hello_world/resources/grit/hello_world_resources_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/webui/webui_util.h"

HelloWorldUI::HelloWorldUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kChromeUIHelloWorldHost);

  webui::SetupWebUIDataSource(source, kHelloWorldResources,
                              IDR_HELLO_WORLD_HELLO_WORLD_HTML);

  source->AddString("platform", version_info::GetOSType());
}

HelloWorldUI::~HelloWorldUI() = default;

HelloWorldDialog::HelloWorldDialog() = default;

void HelloWorldDialog::Show(content::WebUI* web_ui) {
  chrome::ShowWebDialog(web_ui->GetWebContents()->GetNativeView(),
                        Profile::FromWebUI(web_ui), new HelloWorldDialog());
}

ui::mojom::ModalType HelloWorldDialog::GetDialogModalType() const {
  return ui::mojom::ModalType::kWindow;
}

std::u16string HelloWorldDialog::GetDialogTitle() const {
  return u"Hello world";
}

GURL HelloWorldDialog::GetDialogContentURL() const {
  return GURL(kChromeUIHelloWorldURL);
}

void HelloWorldDialog::GetWebUIMessageHandlers(
    std::vector<content::WebUIMessageHandler*>* handlers) {}

void HelloWorldDialog::GetDialogSize(gfx::Size* size) const {
  const int kDefaultWidth = 544;
  const int kDefaultHeight = 628;
  size->SetSize(kDefaultWidth, kDefaultHeight);
}

std::string HelloWorldDialog::GetDialogArgs() const {
  return "";
}

void HelloWorldDialog::OnDialogShown(content::WebUI* webui) {
  webui_ = webui;
}

void HelloWorldDialog::OnDialogClosed(const std::string& json_retval) {
  delete this;
}

void HelloWorldDialog::OnCloseContents(content::WebContents* source,
                                       bool* out_close_dialog) {
  *out_close_dialog = true;
}

bool HelloWorldDialog::ShouldShowDialogTitle() const {
  return true;
}

HelloWorldDialog::~HelloWorldDialog() = default;
