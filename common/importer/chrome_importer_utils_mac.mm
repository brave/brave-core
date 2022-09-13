/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <Cocoa/Cocoa.h>
#include <sys/param.h>

#include "brave/common/importer/chrome_importer_utils.h"

#include "base/files/file_util.h"
#include "base/mac/foundation_util.h"

base::FilePath GetChromeUserDataFolder() {
  base::FilePath result = base::mac::GetUserLibraryPath();
  return result.Append("Application Support/Google/Chrome");
}

base::FilePath GetChromeBetaUserDataFolder() {
  base::FilePath result = base::mac::GetUserLibraryPath();
  return result.Append("Application Support/Google/Chrome Beta");
}

base::FilePath GetChromeDevUserDataFolder() {
  base::FilePath result = base::mac::GetUserLibraryPath();
  return result.Append("Application Support/Google/Chrome Dev");
}

base::FilePath GetCanaryUserDataFolder() {
  base::FilePath result = base::mac::GetUserLibraryPath();
  return result.Append("Application Support/Google/Chrome Canary");
}

base::FilePath GetVivaldiUserDataFolder() {
  base::FilePath result = base::mac::GetUserLibraryPath();
  return result.Append("Application Support/Vivaldi");
}

base::FilePath GetChromiumUserDataFolder() {
  base::FilePath result = base::mac::GetUserLibraryPath();
  return result.Append("Application Support/Chromium");
}

base::FilePath GetEdgeUserDataFolder() {
  base::FilePath result = base::mac::GetUserLibraryPath();
  return result.Append("Application Support/Microsoft Edge");
}

base::FilePath GetOperaUserDataFolder() {
  base::FilePath result = base::mac::GetUserLibraryPath();
  return result.Append("Application Support/com.operasoftware.Opera");
}
