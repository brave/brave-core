/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/localization_util.h"

#include "components/grit/brave_components_strings.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=LocalizationUtilTest*

namespace brave_l10n {

namespace {
constexpr int kInvalidResourceId = -1;
}  // namespace

TEST(LocalizationUtilTest, GetLocalizedResourceUTF16String) {
  const std::u16string localized_resource =
      GetLocalizedResourceUTF16String(IDS_BRAVE_NEW_TAB_STATS);

  EXPECT_EQ(u"Brave Stats", localized_resource);
}

TEST(LocalizationUtilTest,
     GetLocalizedResourceUTF16StringWithInvalidResourceId) {
  const std::u16string localized_resource =
      GetLocalizedResourceUTF16String(kInvalidResourceId);

  EXPECT_TRUE(localized_resource.empty());
}

TEST(LocalizationUtilTest, GetLocalizedResourceUTF16StringWithPlaceholders) {
  int string_id = IDS_TEST_STRING_FOR_PLACEHOLDERS;
  std::vector<std::u16string> placeholders{u"$1", u"$2", u"$3", u"$4"};
  std::vector<size_t> offsets;
  const std::u16string text = brave_l10n::GetStringFUTF16WithPlaceHolders(
      string_id, placeholders, offsets);
  EXPECT_EQ(placeholders.size(), offsets.size());

  // Check |text| doesn't include place holder strings.
  // grd file has above place holders and GetStringFUTF16WithPlaceHolders()
  // returns string w/o them.
  const std::u16string expected_text =
      u"Test string for place holders. Learn more";
  EXPECT_EQ(expected_text, text);
  EXPECT_EQ(offsets[0],
            16ul);  // start offset of "place holders" part in the above.
  EXPECT_EQ(offsets[1], 29ul);  // end offset of "plce holders" part.
  EXPECT_EQ(offsets[2], 31ul);  // start offset of "Learn more" part.
  EXPECT_EQ(offsets[3], 41ul);  // end offset of "Learn more" part.
}

}  // namespace brave_l10n
