/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_account/brave_account_ui_android.h"

#include <memory>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/values.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"

namespace {
class BraveAccountUIMessageHandler : public content::WebUIMessageHandler {
 public:
  BraveAccountUIMessageHandler() = default;

  BraveAccountUIMessageHandler(const BraveAccountUIMessageHandler&) = delete;
  BraveAccountUIMessageHandler& operator=(const BraveAccountUIMessageHandler&) =
      delete;

  ~BraveAccountUIMessageHandler() override = default;

 private:
  // WebUIMessageHandler:
  void RegisterMessages() override;

  void OnDialogCloseMessage(const base::Value::List& args);
};

void BraveAccountUIMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "dialogClose",
      base::BindRepeating(&BraveAccountUIMessageHandler::OnDialogCloseMessage,
                          base::Unretained(this)));
}

void BraveAccountUIMessageHandler::OnDialogCloseMessage(
    const base::Value::List& args) {
  web_ui()->GetWebContents()->Close();
}
}  // namespace

BraveAccountUIAndroid::BraveAccountUIAndroid(content::WebUI* web_ui)
    : BraveAccountUIBase(Profile::FromWebUI(web_ui),
                         base::BindOnce(&webui::SetupWebUIDataSource)),
      WebUIController(web_ui) {
  web_ui->AddMessageHandler(std::make_unique<BraveAccountUIMessageHandler>());
}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveAccountUIAndroid)

BraveAccountUIAndroidConfig::BraveAccountUIAndroidConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kBraveAccountHost) {
  CHECK(brave_account::features::IsBraveAccountEnabled());
}
