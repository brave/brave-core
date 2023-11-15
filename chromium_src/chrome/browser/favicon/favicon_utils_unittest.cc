// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "chrome/browser/favicon/favicon_utils.h"

#include "content/public/browser/navigation_entry.h"
#include "content/public/common/url_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/url_util.h"

namespace favicon {

TEST(BraveFaviconUtilsTest, ShouldThemeifyFaviconForBraveInternalUrl) {
  // Have to manually add the brave scheme since
  // BraveContentClient::AddAdditionalSchemes is not run.
  url::ScopedSchemeRegistryForTests scoped_registry;
  url::AddStandardScheme(content::kBraveUIScheme, url::SCHEME_WITH_HOST);

  std::unique_ptr<content::NavigationEntry> entry =
      content::NavigationEntry::Create();
  const GURL unthemeable_url("brave://wallet");
  const GURL themeable_url("brave://brave-somethingelse");

  entry->SetVirtualURL(unthemeable_url);
  // Brave's override for some brave-internal urls should not be themeable.
  EXPECT_FALSE(ShouldThemifyFaviconForEntry(entry.get()));

  entry->SetVirtualURL(themeable_url);
  // Brave's override should not interfere with other themeable urls.
  EXPECT_TRUE(ShouldThemifyFaviconForEntry(entry.get()));
}

}  // namespace favicon
