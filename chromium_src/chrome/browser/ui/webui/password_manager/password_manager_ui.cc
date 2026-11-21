/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_unscaled_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/webui/webui_util.h"

namespace {

void AddBravePasswordManagerSourceData(content::WebUIDataSource* source) {
  source->AddResourcePath("images/password_manager_logo.svg",
                          IDR_BRAVE_PASSWORD_MANAGER_LOGO);
  source->AddLocalizedString("bravePasswordManagerFillLabel",
                             IDS_BRAVE_PASSWORD_MANAGER_UI_FILL_LABEL);
  source->AddLocalizedString("bravePasswordManagerFillSubLabel",
                             IDS_BRAVE_PASSWORD_MANAGER_UI_FILL_SUB_LABEL);
}

}  // namespace

#include <chrome/browser/ui/webui/password_manager/password_manager_ui.cc>
