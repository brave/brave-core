/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/web_applications/components/web_app_file_handler_registration.h"

namespace web_app {

bool ShouldRegisterFileHandlersWithOs() {
  return false;
}

void RegisterFileHandlersWithOs(const AppId& app_id,
                                const std::string& app_name,
                                Profile* profile,
                                const apps::FileHandlers& file_handlers) {
}

void UnregisterFileHandlersWithOs(const AppId& app_id, Profile* profile) {
}

}  // namespace web_app
