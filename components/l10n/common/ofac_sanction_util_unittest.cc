/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/ofac_sanction_util.h"

#include <string>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/l10n/common/locale_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=OFACSanctionUtilTest*

namespace brave_l10n {

namespace {

struct ParamInfo final {
  std::string locale;
  bool should_sanction_un_m49_codes;
  std::string expected_language_code;
  std::string expected_country_code;
  bool expected_is_ofac_sanctioned;
} kTests[] = {
    {{}, false, "en", "US", false},
    {{}, true, "en", "US", false},

    // ISO 639-1 language codes, see
    // https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes.

    {"en", false, "en", "US", false},
    {"en", true, "en", "US", false},

    // ISO 639-1 language codes and ISO 3166-1 alpha-2 country codes, see
    // https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes and
    // https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2.

    {"en_US", false, "en", "US", false},
    {"en_US", true, "en", "US", false},

    // Belarus.
    {"en_BY", false, "en", "BY", true},
    {"en_BY", true, "en", "BY", true},

    // Cuba.
    {"en_CU", false, "en", "CU", true},
    {"en_CU", true, "en", "CU", true},

    // Iran.
    {"en_IR", false, "en", "IR", true},
    {"en_IR", true, "en", "IR", true},

    // Moldova.
    {"en_MD", false, "en", "MD", true},
    {"en_MD", true, "en", "MD", true},

    // North Korea.
    {"en_KP", false, "en", "KP", true},
    {"en_KP", true, "en", "KP", true},

    // Russia.
    {"en_RU", false, "en", "RU", true},
    {"en_RU", true, "en", "RU", true},

    // Syria.
    {"en_SY", false, "en", "SY", true},
    {"en_SY", true, "en", "SY", true},

    // Ukraine.
    {"en_UA", false, "en", "UA", true},
    {"en_UA", true, "en", "UA", true},

    // ISO 639-1 language codes and ISO 3166-1 numeric-3 country codes, see
    // https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes and
    // https://en.wikipedia.org/wiki/ISO_3166-1_numeric.

    {"en_840", false, "en", "840", false},
    {"en_840", true, "en", "840", false},

    // Belarus.
    {"en_112", false, "en", "112", true},
    {"en_112", true, "en", "112", true},

    // Cuba.
    {"en_192", false, "en", "192", true},
    {"en_192", true, "en", "192", true},

    // Iran.
    {"en_364", false, "en", "364", true},
    {"en_364", true, "en", "364", true},

    // Moldova.
    {"en_498", false, "en", "498", true},
    {"en_498", true, "en", "498", true},

    // North Korea.
    {"en_408", false, "en", "408", true},
    {"en_408", true, "en", "408", true},

    // Russia.
    {"en_643", false, "en", "643", true},
    {"en_643", true, "en", "643", true},

    // Syria.
    {"en_760", false, "en", "760", true},
    {"en_760", true, "en", "760", true},

    // Ukraine.
    {"en_804", false, "en", "804", true},
    {"en_804", true, "en", "804", true},

    // ISO 639-1 language codes and UN M.49 codes, see
    // https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes and
    // https://en.wikipedia.org/wiki/UN_M49.

    // World which includes sanctioned Belarus, Cuba, Iran, Moldova, North
    // Korea, Russia, Syria and Ukraine.
    {"en_001", false, "en", "001", false},
    {"en_001", true, "en", "001", true},

    // Caribbean which includes sanctioned Cuba.
    {"en_029", false, "en", "029", false},
    {"en_029", true, "en", "029", true},

    // Eastern Asia which includes sanctioned North Korea.
    {"en_030", false, "en", "030", false},
    {"en_030", true, "en", "030", true},

    // Southern Asia which includes sanctioned Iran.
    {"en_034", false, "en", "034", false},
    {"en_034", true, "en", "034", true},

    // Western Asia which includes sanctioned Syria.
    {"en_145", false, "en", "145", false},
    {"en_145", true, "en", "145", true},

    // Eastern Europe (including Northern Asia) which includes sanctioned
    // Belarus, Moldova, Russia and Ukraine.
    {"en_151", false, "en", "151", false},
    {"en_151", true, "en", "151", true}};

}  // namespace

class OFACSanctionUtilTest : public testing::Test,
                             public testing::WithParamInterface<ParamInfo> {};

TEST_P(OFACSanctionUtilTest, Locale) {
  // Arrange
  const ParamInfo param = GetParam();

  const std::string locale = param.locale;

  // Act
  const std::string language_code = GetISOLanguageCode(locale);

  const std::string country_code = GetISOCountryCode(locale);

  bool is_ofac_sanctioned = IsISOCountryCodeOFACSanctioned(country_code);
  if (!is_ofac_sanctioned && param.should_sanction_un_m49_codes) {
    is_ofac_sanctioned = IsUNM49CodeOFACSanctioned(country_code);
  }

  // Assert
  EXPECT_EQ(param.expected_language_code, language_code);
  EXPECT_EQ(param.expected_country_code, country_code);
  EXPECT_EQ(param.expected_is_ofac_sanctioned, is_ofac_sanctioned);
}

std::string TestParamToString(
    const testing::TestParamInfo<ParamInfo>& test_param) {
  const std::string is_ofac_sanctioned =
      test_param.param.expected_is_ofac_sanctioned ? "ShouldSanction"
                                                   : "ShouldNotSanction";

  std::string locale = test_param.param.locale;
  base::ReplaceChars(locale, "-.", "_", &locale);
  if (locale.empty()) {
    locale = "Empty";
  }

  const std::string should_sanction_un_m49_codes =
      test_param.param.should_sanction_un_m49_codes
          ? "WhenShouldSanctionUNM49CodesIsSetToTrue"
          : "WhenShouldSanctionUNM49CodesIsSetToFalse";

  return base::StringPrintf("%s_%s_%s", is_ofac_sanctioned.c_str(),
                            locale.c_str(),
                            should_sanction_un_m49_codes.c_str());
}

INSTANTIATE_TEST_SUITE_P(,
                         OFACSanctionUtilTest,
                         testing::ValuesIn(kTests),
                         TestParamToString);

}  // namespace brave_l10n
