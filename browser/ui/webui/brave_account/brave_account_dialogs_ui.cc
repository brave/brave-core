/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_account/brave_account_dialogs_ui.h"

#include "base/functional/bind.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"

BraveAccountDialogsUI::BraveAccountDialogsUI(content::WebUI* web_ui)
    : BraveAccountDialogsUIBase(Profile::FromWebUI(web_ui),
                                base::BindOnce(&webui::SetupWebUIDataSource)),
      ConstrainedWebDialogUI(web_ui) {}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveAccountDialogsUI)

BraveAccountDialogsUIConfig::BraveAccountDialogsUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kBraveAccountDialogsHost) {}

bool BraveAccountDialogsUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return false;
}
