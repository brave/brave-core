/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_account/brave_account_ui.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"

BraveAccountUI::BraveAccountUI(content::WebUI* web_ui)
    : BraveAccountUIBase(Profile::FromWebUI(web_ui),
                         base::BindOnce(&webui::SetupWebUIDataSource)),
      ConstrainedWebDialogUI(web_ui) {}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveAccountUI)

BraveAccountUIConfig::BraveAccountUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kBraveAccountHost) {
  CHECK(brave_account::features::IsBraveAccountEnabled());
}
