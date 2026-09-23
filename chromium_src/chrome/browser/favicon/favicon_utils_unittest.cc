// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/favicon/favicon_utils.h"

#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "content/public/browser/navigation_entry.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace favicon {

TEST(BraveFaviconUtilsTest, ShouldThemeifyFaviconForBraveInternalUrl) {
  std::unique_ptr<content::NavigationEntry> entry =
      content::NavigationEntry::Create();
  const GURL unthemeable_url_wallet("chrome://wallet");
  const GURL unthemeable_url_newtab("chrome://newtab");
  const GURL themeable_url("chrome://brave-somethingelse");

// Brave's override for some brave-internal urls should not be themeable.
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  entry->SetVirtualURL(unthemeable_url_wallet);
  EXPECT_FALSE(ShouldThemifyFaviconForEntry(entry.get()));
#endif
  entry->SetVirtualURL(unthemeable_url_newtab);
  EXPECT_FALSE(ShouldThemifyFaviconForEntry(entry.get()));

  // Brave's override should not interfere with other themeable urls.
  entry->SetVirtualURL(themeable_url);
  EXPECT_TRUE(ShouldThemifyFaviconForEntry(entry.get()));
}

}  // namespace favicon
