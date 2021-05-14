/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/app_launcher_login_handler.h"
#include "base/values.h"

#define RegisterMessages RegisterMessages_ChromiumImpl
#include "../../../../../../chrome/browser/ui/webui/app_launcher_login_handler.cc"
#undef RegisterMessages

namespace {
void DummyCallback() {}
void DummyWebUICallback(const base::ListValue* args) {}
}  // namespace

void AppLauncherLoginHandler::RegisterMessages() {
  // WebUIMessageCallbacks are emplaced in the map
  // if the key exists in the WebUIs message_callback_
  // insertion will be ignored
  web_ui()->RegisterMessageCallback("initializeSyncLogin",
                                    base::BindRepeating(&DummyWebUICallback));

  AppLauncherLoginHandler::RegisterMessages_ChromiumImpl();
  profile_info_watcher_ = std::make_unique<ProfileInfoWatcher>(
      Profile::FromWebUI(web_ui()), base::BindRepeating(DummyCallback));
}
