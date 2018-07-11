/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_CHROME_PROFILE_LOCK_H__
#define BRAVE_BROWSER_IMPORTER_CHROME_PROFILE_LOCK_H__

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "brave/browser/importer/browser_profile_lock.h"
#include "chrome/browser/process_singleton.h"

class ChromeProfileLock : public BrowserProfileLock {
 public:
  explicit ChromeProfileLock(const base::FilePath& user_data_dir);
  ~ChromeProfileLock() override;

  // Locks and releases the profile.
  void Lock() override;
  void Unlock() override;

  // Returns true if we lock the profile successfully.
  bool HasAcquired() override;

 private:
  bool lock_acquired_;
  base::FilePath user_data_dir_;
  std::unique_ptr<ProcessSingleton> process_singleton_;

  bool NotificationCallback(const base::CommandLine& command_line,
                            const base::FilePath& current_directory);

  DISALLOW_COPY_AND_ASSIGN(ChromeProfileLock);
};

#endif  // BRAVE_BROWSER_IMPORTER_CHROME_PROFILE_LOCK_H__
