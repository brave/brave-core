/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_settings_default_browser_handler.h"

#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#include "brave/browser/brave_shell_integration_win.h"
#endif

namespace settings {

BraveDefaultBrowserHandler::~BraveDefaultBrowserHandler() = default;

void BraveDefaultBrowserHandler::SetAsDefaultBrowser(
    const base::Value::List& args) {
  DefaultBrowserHandler::SetAsDefaultBrowser(args);

#if BUILDFLAG(IS_WIN)
  // Trying to pin when user ask this as a default browser.
  shell_integration::win::PinToTaskbar(Profile::FromWebUI(web_ui()));
#endif
}

}  // namespace settings
