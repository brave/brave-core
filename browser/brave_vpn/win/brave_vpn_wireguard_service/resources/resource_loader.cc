/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/resources/resource_loader.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/i18n/rtl.h"
#include "base/path_service.h"
#include "ui/base/resource/resource_bundle.h"

namespace brave_vpn {

namespace {
base::FilePath GetResourcesPakFilePath(base::FilePath assets_path,
                                       const std::string& locale) {
  auto pak_path = assets_path;
  pak_path = pak_path.AppendASCII("Locales");
  pak_path = pak_path.AppendASCII(locale + ".pak");
  return pak_path;
}

}  // namespace

base::FilePath FindPakFilePath(const base::FilePath& assets_path,
                               const std::string& locale) {
  auto pak_path = GetResourcesPakFilePath(assets_path.DirName(), locale);
  if (base::PathExists(pak_path)) {
    return pak_path;
  }
  pak_path = GetResourcesPakFilePath(assets_path, locale);
  if (base::PathExists(pak_path)) {
    return pak_path;
  }
  if (locale != "en-US") {
  }
  CHECK_NE(locale, "en-US");
  return FindPakFilePath(assets_path, "en-US");
}

void LoadLocaleResources() {
  base::FilePath assets_path;
  base::PathService::Get(base::DIR_ASSETS, &assets_path);
  auto pak_path =
      FindPakFilePath(assets_path, base::i18n::GetConfiguredLocale());
  CHECK(base::PathExists(pak_path));
  ui::ResourceBundle::InitSharedInstanceWithPakPath(pak_path);
}

}  //  namespace brave_vpn
