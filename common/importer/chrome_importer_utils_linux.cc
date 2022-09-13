/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/importer/chrome_importer_utils.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"

base::FilePath GetChromeUserDataFolder() {
  base::FilePath result;
  if (!base::PathService::Get(base::DIR_HOME, &result))
    return base::FilePath();

  result = result.Append(".config");
  result = result.Append("google-chrome");

  return result;
}

base::FilePath GetChromeBetaUserDataFolder() {
  base::FilePath result;
  if (!base::PathService::Get(base::DIR_HOME, &result))
    return base::FilePath();

  result = result.Append(".config");
  result = result.Append("google-chrome-beta");

  return result;
}

base::FilePath GetChromeDevUserDataFolder() {
  base::FilePath result;
  if (!base::PathService::Get(base::DIR_HOME, &result))
    return base::FilePath();

  result = result.Append(".config");
  result = result.Append("google-chrome-unstable");

  return result;
}

base::FilePath GetVivaldiUserDataFolder() {
  base::FilePath result;
  if (!base::PathService::Get(base::DIR_HOME, &result))
    return base::FilePath();

  result = result.Append(".config");
  result = result.Append("vivaldi");

  return result;
}

base::FilePath GetChromiumUserDataFolder() {
  base::FilePath result;
  if (!base::PathService::Get(base::DIR_HOME, &result))
    return base::FilePath();

  result = result.Append(".config");
  result = result.Append("chromium");

  return result;
}

base::FilePath GetEdgeUserDataFolder() {
  base::FilePath result;
  if (!base::PathService::Get(base::DIR_HOME, &result))
    return base::FilePath();

  result = result.Append(".config");
  result = result.Append("microsoft-edge");

  return result;
}

base::FilePath GetOperaUserDataFolder() {
  base::FilePath result;
  if (!base::PathService::Get(base::DIR_HOME, &result))
    return base::FilePath();

  result = result.Append(".config");
  result = result.Append("opera");

  return result;
}

base::FilePath GetOperaSnapUserDataFolder() {
  base::FilePath result;
  if (!base::PathService::Get(base::DIR_HOME, &result))
    return base::FilePath();

  result = result.Append("snap");
  result = result.Append("opera");
  result = result.Append("current");
  result = result.Append(".config");
  result = result.Append("opera");

  return result;
}
