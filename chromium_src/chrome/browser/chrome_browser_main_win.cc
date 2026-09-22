/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

class PrefService;

namespace base {
class FilePath;
}  // namespace base

namespace brave {

// The restore is spliced into PreCreateMainMessageLoop, ahead of
// os_crypt_async::Init(), by rewrite/chrome/browser/chrome_browser_main_win.cc
// .yaml. Defined in brave/browser/os_crypt/os_crypt_key_backup.cc, and declared
// here so that chrome/browser does not depend on Brave targets. How the restore
// went is recorded in `Local State`, not returned.
void MaybeRestoreOSCryptKey(const base::FilePath& user_data_dir,
                            PrefService* local_state);

}  // namespace brave

#include <chrome/browser/chrome_browser_main_win.cc>
