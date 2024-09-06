// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/bookmarks/bookmark_utils.h"

#include "content/public/common/url_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace chrome {
TEST(BraveBookmarkUtilsTest, BraveSchemeIsReplaced) {
  GURL url(content::kChromeUIScheme + std::string("://test"));
  std::u16string new_url = chrome::FormatBookmarkURLForDisplay(url);
  EXPECT_EQ(GURL(new_url).scheme(), content::kBraveUIScheme);
}
}  // namespace chrome
