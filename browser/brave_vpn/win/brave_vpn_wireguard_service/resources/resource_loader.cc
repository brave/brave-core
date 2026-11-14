/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/resources/resource_loader.h"

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"

namespace brave_vpn {

base::FilePath FindLocalesDir(const base::FilePath& assets_path) {
  base::FilePath locales_dir = assets_path.DirName().AppendASCII("Locales");
  if (base::DirectoryExists(locales_dir)) {
    return locales_dir;
  }
  return assets_path.AppendASCII("Locales");
}

std::string SelectLocale(const base::FilePath& assets_path) {
  CHECK(base::PathService::Override(ui::DIR_LOCALES,
                                    FindLocalesDir(assets_path)));
  return l10n_util::GetApplicationLocale(/*pref_locale=*/"");
}

void LoadLocaleResources() {
  base::FilePath assets_path;
  base::PathService::Get(base::DIR_ASSETS, &assets_path);
  const std::string locale = SelectLocale(assets_path);
  CHECK(!locale.empty());
  ui::ResourceBundle::InitSharedInstanceWithPakPath(
      ui::ResourceBundle::GetLocaleFilePath(locale));
}

}  //  namespace brave_vpn
