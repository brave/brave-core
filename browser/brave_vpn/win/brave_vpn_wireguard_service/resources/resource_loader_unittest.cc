/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/resources/resource_loader.h"

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "brave/components/constants/brave_paths.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/icu/source/common/unicode/locid.h"

namespace brave_vpn {

TEST(ResourceLoaderTest, FindPakPath) {
  base::FilePath test_data_dir;
  base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
  base::FilePath wireguard = test_data_dir.Append(L"wireguard");
  // Looking to upper directory.
  EXPECT_EQ(
      FindPakFilePath(wireguard.Append(L"BraveVpnWireguardService"), "en-US"),
      base::FilePath(wireguard.Append(L"Locales").Append(L"en-US.pak")));
  // Looking to current directory.
  EXPECT_EQ(FindPakFilePath(wireguard, "en-US"),
            base::FilePath(wireguard.Append(L"Locales").Append(L"en-US.pak")));
  // Fallback to english locale.
  EXPECT_EQ(FindPakFilePath(wireguard, "de-DE"),
            base::FilePath(wireguard.Append(L"Locales").Append(L"en-US.pak")));
}

TEST(ResourceLoaderTest, GetSystemLocale) {
  const icu::Locale original = icu::Locale::getDefault();
  for (const auto [icu_name, expected] :
       {std::pair{"en_US", "en-US"}, std::pair{"de", "de"},
        std::pair{"es_419", "es-419"}, std::pair{"pt_BR", "pt-BR"},
        std::pair{"zh_CN", "zh-CN"}, std::pair{"zh_TW", "zh-TW"}}) {
    SCOPED_TRACE(icu_name);
    UErrorCode status = U_ZERO_ERROR;
    icu::Locale::setDefault(icu::Locale(icu_name), status);
    ASSERT_TRUE(U_SUCCESS(status));
    EXPECT_EQ(GetSystemLocale(), expected);
  }
  UErrorCode status = U_ZERO_ERROR;
  icu::Locale::setDefault(original, status);
}

}  // namespace brave_vpn
