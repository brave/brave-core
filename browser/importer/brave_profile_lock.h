/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IMPORTER_BRAVE_PROFILE_LOCK_H__
#define BRAVE_BROWSER_IMPORTER_BRAVE_PROFILE_LOCK_H__

#include "base/files/file_path.h"
#include "brave/browser/importer/chrome_profile_lock.h"

class BraveProfileLock : public ChromeProfileLock {
 public:
  explicit BraveProfileLock(const base::FilePath& user_data_dir);
  ~BraveProfileLock() override;
};

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_PROFILE_LOCK_H__
