/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/upgrade_detector/get_installed_version.h"

#include <utility>

#include "base/functional/callback.h"
#include "base/strings/utf_string_conversions.h"
#include "base/version.h"
#include "brave/browser/mac/keystone_glue.h"
#include "brave/browser/mac_features.h"
#include "chrome/browser/updater/browser_updater_client_util.h"
#include "chrome/common/chrome_features.h"
#include "components/version_info/version_info.h"

namespace {

InstalledAndCriticalVersion GetInstalledVersionSynchronous() {
  if (!keystone_glue::KeystoneEnabled()) {
    return InstalledAndCriticalVersion(version_info::GetVersion());
  }
  return InstalledAndCriticalVersion(base::Version(
      base::UTF16ToASCII(keystone_glue::CurrentlyInstalledVersion())));
}

}  // namespace

#define GetInstalledVersion GetInstalledVersion_ChromiumImpl
#include "src/chrome/browser/upgrade_detector/get_installed_version_mac.mm"
#undef GetInstalledVersion

void GetInstalledVersion(InstalledVersionCallback callback) {
  if (brave::ShouldUseOmaha4()) {
    GetInstalledVersion_ChromiumImpl(std::move(callback));
  } else {
    std::move(callback).Run(GetInstalledVersionSynchronous());
  }
}
