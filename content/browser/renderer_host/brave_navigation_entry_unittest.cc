/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>

#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/common/referrer.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace content {

class BraveNavigationEntryTest : public testing::Test {
 private:
  BrowserTaskEnvironment task_environment_;
  TestBrowserContext browser_context_;

 protected:
  std::unique_ptr<NavigationEntry> CreateEntry(const GURL& url) {
    return NavigationController::CreateNavigationEntry(
        url, Referrer(), /* initiator_origin= */ std::nullopt,
        /* initiator_base_url= */ std::nullopt, ui::PAGE_TRANSITION_TYPED,
        /* is_renderer_initiated= */ false, /* extra_headers= */ std::string(),
        &browser_context_, /* blob_url_loader_factory= */ nullptr);
  }
};

TEST_F(BraveNavigationEntryTest,
       GetTitleForDisplayConvertsChromeSchemeToBrave) {
  auto entry = CreateEntry(GURL("chrome://settings"));
  EXPECT_EQ(u"brave://settings", entry->GetTitleForDisplay());

  entry = CreateEntry(GURL("chrome://history"));
  EXPECT_EQ(u"brave://history", entry->GetTitleForDisplay());

  entry = CreateEntry(GURL("chrome://flags"));
  EXPECT_EQ(u"brave://flags", entry->GetTitleForDisplay());
}

TEST_F(BraveNavigationEntryTest, GetTitleForDisplayPreservesExplicitTitle) {
  auto entry = CreateEntry(GURL("chrome://settings"));
  entry->SetTitle(u"Settings");
  EXPECT_EQ(u"Settings", entry->GetTitleForDisplay());
}

TEST_F(BraveNavigationEntryTest,
       GetTitleForDisplayDoesNotAffectNonChromeScheme) {
  auto entry = CreateEntry(GURL("https://example.com"));
  EXPECT_EQ(u"example.com", entry->GetTitleForDisplay());

  entry = CreateEntry(GURL("brave://settings"));
  EXPECT_EQ(u"brave://settings", entry->GetTitleForDisplay());

  entry = CreateEntry(GURL("http://chrome.com"));
  EXPECT_EQ(u"chrome.com", entry->GetTitleForDisplay());

  entry = CreateEntry(GURL("http://example.com/?chrome://settings"));
  EXPECT_EQ(u"example.com/?chrome://settings", entry->GetTitleForDisplay());
}

}  // namespace content
