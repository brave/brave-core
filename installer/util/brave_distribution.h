/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_INSTALLER_UTIL_BRAVE_DISTRIBUTION_H_
#define BRAVE_INSTALLER_UTIL_BRAVE_DISTRIBUTION_H_

#include "chrome/installer/util/google_chrome_distribution.h"

class BraveDistribution : public GoogleChromeDistribution {
 public:
  void DoPostUninstallOperations(
      const base::Version& version,
      const base::FilePath& local_data_path,
      const base::string16& distribution_data) override { };

  BraveDistribution() { };

  DISALLOW_COPY_AND_ASSIGN(BraveDistribution);
};

#endif // BRAVE_INSTALLER_UTIL_BRAVE_DISTRIBUTION_H_
