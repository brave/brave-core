/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/installer/setup/install.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/time/time.h"
#include "chrome/installer/util/shell_util.h"

/*
 * Work around crbug.com/331836635 "Double prompt to pin to taskbar on Windows".
 *
 * Upstream pins the browser to the taskbar both in mini_installer and during
 * first run. Our online installer runs mini_installer and then immediately
 * launches the browser. This makes two "Do you want to pin Brave to your
 * taskbar?" notifications appear.
 *
 * The solution implemented here only pins on first run if the browser
 * installation is more than 3 minutes old. This avoids the second notification
 * in the scenario above, while still giving non-admin users in system-wide
 * installations a chance to pin.
 */

namespace {

constexpr int kPinAfterMins = 3;

bool ShouldPinToTaskbar(bool do_not_create_taskbar_shortcut,
                        ShellUtil::ShortcutOperation shortcut_operation,
                        const base::FilePath& target) {
  if (do_not_create_taskbar_shortcut) {
    return false;
  }
  if (shortcut_operation == ShellUtil::SHELL_SHORTCUT_CREATE_ALWAYS) {
    // We are inside a full browser installation process.
    return true;
  } else {
    // This code runs on first launch. The if (...) branch may have just run
    // during the installation process. Don't show another "Do you want to pin?"
    // notification unless some time has passed.
    base::File::Info info;
    if (base::GetFileInfo(target, &info)) {
      base::TimeDelta installation_age = base::Time::Now() - info.creation_time;
      return installation_age.InMinutes() >= kPinAfterMins;
    } else {
      return false;
    }
  }
}

}  // namespace

#define set_pin_to_taskbar(...)                                         \
  set_pin_to_taskbar(ShouldPinToTaskbar(do_not_create_taskbar_shortcut, \
                                        shortcut_operation, target))

#include "src/chrome/installer/setup/install.cc"

#undef set_pin_to_taskbar
