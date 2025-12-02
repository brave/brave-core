/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/install_static/user_data_dir.h"

#include "base/strings/string_util.h"
#include "base/strings/string_view_util.h"
#include "chrome/install_static/install_util.h"

namespace install_static {

std::wstring& BraveAppendChromeInstallSubDirectory(const InstallConstants& mode,
                                                   bool include_suffix,
                                                   std::wstring* path);

}  // namespace install_static

#define AppendChromeInstallSubDirectory BraveAppendChromeInstallSubDirectory

#include <chrome/install_static/user_data_dir.cc>

#undef AppendChromeInstallSubDirectory

namespace install_static {

std::wstring& BraveAppendChromeInstallSubDirectory(const InstallConstants& mode,
                                                   bool include_suffix,
                                                   std::wstring* path) {
  AppendChromeInstallSubDirectory(mode, include_suffix, path);
  // Special case to handle the Policy version of the path for Brave.
  // Brave uses `SOFTWARE\Policies\BraveSoftware\Brave`
  // instead of `SOFTWARE\Policies\BraveSoftware\Brave-Browser`
  if (!include_suffix && base::StartsWith(*path, L"SOFTWARE\\Policies\\") &&
      base::EndsWith(*path, kProductPathName)) {
    *path = path->substr(0, (path->length() - kProductPathNameLength));
    path->append(L"Brave");
  }

  return *path;
}

}  // namespace install_static
