/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/resources/resource_loader.h"

#include "base/files/file_path.h"
#include "base/i18n/icubridge/default_icu_locale.h"
#include "base/i18n/language_tag.h"
#include "base/i18n/test/scoped_icu_locale.h"
#include "base/path_service.h"
#include "base/test/scoped_path_override.h"
#include "brave/components/constants/brave_paths.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/ui_base_paths.h"

namespace brave_vpn {

namespace {

base::FilePath GetWireguardTestDataDir() {
  base::FilePath test_data_dir;
  base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
  return test_data_dir.Append(L"wireguard");
}

}  // namespace

TEST(ResourceLoaderTest, FindLocalesDir) {
  const base::FilePath wireguard = GetWireguardTestDataDir();
  // Looking to upper directory.
  EXPECT_EQ(FindLocalesDir(wireguard.Append(L"BraveVpnWireguardService")),
            wireguard.Append(L"Locales"));
  // Looking to current directory.
  EXPECT_EQ(FindLocalesDir(wireguard), wireguard.Append(L"Locales"));
}

TEST(ResourceLoaderTest, SelectLocaleFallsBackToEnglishAndSetsIcu) {
  base::ScopedPathOverride locales_dir(ui::DIR_LOCALES);
  base::i18n::ScopedDefaultIcuLocale icu_locale(
      base::i18n::GetKnownLanguageTag("fr"));

  // The test data has only an "en-US" pak, whatever the OS prefers.
  EXPECT_EQ(SelectLocale(GetWireguardTestDataDir()), "en-US");
  EXPECT_EQ(base::i18n::GetDefaultIcuLocale().tag_string(), "en-US");
}

}  // namespace brave_vpn
