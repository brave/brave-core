/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/chrome_profile_lock.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/process_singleton.h"

ChromeProfileLock::ChromeProfileLock(const base::FilePath& user_data_dir)
    : lock_acquired_(false),
      process_singleton_(new ProcessSingleton(
          user_data_dir,
          base::Bind(&ChromeProfileLock::NotificationCallback,
                     base::Unretained(this)))),
      user_data_dir_(user_data_dir) {}

ChromeProfileLock::~ChromeProfileLock() {
  Unlock();
}

void ChromeProfileLock::Lock() {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  if (HasAcquired())
    return;
  lock_acquired_ = process_singleton_->Create();
}

void ChromeProfileLock::Unlock() {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  if (!HasAcquired())
    return;
  process_singleton_->Cleanup();
  process_singleton_.reset(
      new ProcessSingleton(user_data_dir_,
                           base::Bind(&ChromeProfileLock::NotificationCallback,
                                      base::Unretained(this))));
  lock_acquired_ = false;
}

bool ChromeProfileLock::HasAcquired() {
  return lock_acquired_;
}

bool ChromeProfileLock::NotificationCallback(
    const base::CommandLine& command_line,
    const base::FilePath& current_directory) {
  return false;
}
