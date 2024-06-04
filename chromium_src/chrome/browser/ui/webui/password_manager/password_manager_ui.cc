/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/navigation_bar_data_provider.h"
#include "brave/grit/brave_unscaled_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "content/public/browser/web_ui_data_source.h"

namespace {

// Called from the original password_manager_ui.cc's
// CreateAndAddPasswordsUIHTMLSource via a patch.
void BraveAddPasswordManagerResources(content::WebUIDataSource* source,
                                      Profile* profile) {
  NavigationBarDataProvider::Initialize(source, profile);
}

}  // namespace

#define SetupWebUIDataSource(...)                             \
  SetupWebUIDataSource(__VA_ARGS__);                          \
  source->AddResourcePath("images/password_manager_logo.svg", \
                          IDR_BRAVE_PASSWORD_MANAGER_LOGO);   \
  BraveAddPasswordManagerResources(source, profile);

#include "src/chrome/browser/ui/webui/password_manager/password_manager_ui.cc"

#undef SetupWebUIDataSource
