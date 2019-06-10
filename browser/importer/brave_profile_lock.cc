/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/brave_profile_lock.h"

#include "base/logging.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"

BraveProfileLock::BraveProfileLock(const base::FilePath& user_data_dir)
    : ChromeProfileLock(user_data_dir) {}

BraveProfileLock::~BraveProfileLock() {}

void BraveProfileLock::Lock() {
#if defined(OS_WIN)
  ChromeProfileLock::Lock();
#elif defined(OS_POSIX)
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  if (HasAcquired())
    return;

  ProcessSingleton::NotifyResult rv =
      process_singleton_->NotifyOtherProcessOrCreate();
  LOG(INFO) << "BraveProfileLock::Lock: NotifyOtherProcessOrCreate rv: " << rv;
  lock_acquired_ = rv == ProcessSingleton::PROCESS_NONE;
#endif
}
