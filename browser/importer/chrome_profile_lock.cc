/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/chrome_profile_lock.h"

#include "base/bind.h"
#include "base/bind_helpers.h"

ChromeProfileLock::ChromeProfileLock(
    const base::FilePath& user_data_dir)
    : lock_acquired_(false),
      process_singleton_(user_data_dir,
                         base::Bind(&ChromeProfileLock::NotificationCallback,
				    base::Unretained(this))) {
  Lock();
}

ChromeProfileLock::~ChromeProfileLock() {
  Unlock();
}

void ChromeProfileLock::Lock() {
  if (HasAcquired())
    return;
  lock_acquired_ = process_singleton_.Create();
}

void ChromeProfileLock::Unlock() {
  if (!HasAcquired())
    return;
  process_singleton_.Cleanup();
}

bool ChromeProfileLock::HasAcquired() {
  return lock_acquired_;
}

bool ChromeProfileLock::NotificationCallback(
    const base::CommandLine& command_line,
    const base::FilePath& current_directory) {
  return false;
}
