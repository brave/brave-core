/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/updater/browser_updater_client_util.h"

#include <string_view>

#include "base/files/file_path.h"
#include "chrome/updater/updater_branding.h"

#undef FILE_PATH_LITERAL

std::string_view FILE_PATH_LITERAL(std::string_view path) {
  // Chromium < 145.0.7564.0 appends "SoftwareUpdate" to
  // COMPANY_SHORTNAME_STRING. This results in "BraveSoftwareSoftwareUpdate"
  // (2x "Software"). Use KEYSTONE_NAME instead to get "BraveSoftwareUpdate".
  // TODO(https://github.com/brave/brave-browser/issues/51355): Remove this once
  // Brave is on Chromium 145.0.7564.0+.
  if (path == COMPANY_SHORTNAME_STRING "SoftwareUpdate") {
    return KEYSTONE_NAME;
  }
  return path;
}

#include <chrome/browser/updater/browser_updater_client_util_mac.mm>

// Upstream's original definition:
#define FILE_PATH_LITERAL(x) x
