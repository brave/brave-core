/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/installer/setup/brand_behaviors.h"

#define DoPostUninstallOperations DoPostUninstallOperations_UNUSED
#include "../../../../../chrome/installer/setup/google_chrome_behaviors.cc"
#undef DoPostUninstallOperations

namespace installer {

void DoPostUninstallOperations(const base::Version& version,
                               const base::FilePath& local_data_path,
                               const std::u16string& distribution_data) {
  // Brave browser doesn't launch uninstall survey page.
}

}
