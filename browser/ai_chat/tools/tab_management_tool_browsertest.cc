// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/tab_management_tool.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface_iterator.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/tab_groups/tab_group_id.h"
#include "components/tab_groups/tab_group_visual_data.h"
#include "components/tabs/public/tab_group.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

// Helper to extract the single text content block string from a ToolResult
std::string ExtractToolOutputText(
    const std::vector<mojom::ContentBlockPtr>& result) {
  if (result.empty() || !result[0]->is_text_content_block()) {
    return std::string();
  }
  return result[0]->get_text_content_block()->text;
}

// Helper to run the tool with optional client_data and return the output text
std::string RunToolAndGetText(base::Location location,
                              TabManagementTool* tool,
                              const std::string& input_json) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  base::test::TestFuture<std::vector<mojom::ContentBlockPtr>,
                         std::vector<mojom::ToolArtifactPtr>>
      future;
  tool->UseTool(input_json, future.GetCallback());
  return ExtractToolOutputText(
      future.Get<std::vector<mojom::ContentBlockPtr>>());
}

int GetTabHandle(content::WebContents* contents) {
  if (auto* tab_interface = tabs::TabInterface::GetFromContents(contents)) {
    return tab_interface->GetHandle().raw_value();
  }
  return -1;  // Invalid handle
}

// Build a minimal expected windows skeleton for the current profile state so
// tests can compare. We should always also test actual browser state to verify
// the response has waited for the state to catch up with the commands.
base::DictValue BuildExpectedOutputSubset(Profile* profile) {
  base::DictValue expected;
  base::ListValue expected_windows;
  for (BrowserWindowInterface* bwi : GetAllBrowserWindowInterfaces()) {
    if (bwi->GetProfile() != profile) {
      continue;
    }
    // Manually skip empty windows to verify that the tool doesn't
    // provide any windows with empty tab strips. It should do that
    // via timing so that we're sure we're sending accurate window-tab
    // relationships.

    if (bwi->GetTabStripModel()->empty()) {
      continue;
    }
    base::DictValue w;
    w.Set("window_id", bwi->GetSessionID().id());
    w.Set("active_tab_index", bwi->GetTabStripModel()->active_index());

    base::ListValue tabs;
    for (int i = 0; i < bwi->GetTabStripModel()->count(); ++i) {
      content::WebContents* wc = bwi->GetTabStripModel()->GetWebContentsAt(i);
      base::DictValue t;
      t.Set("tab_id", GetTabHandle(wc));
      t.Set("index", i);
      t.Set("is_active", i == bwi->GetTabStripModel()->active_index());
      if (bwi->GetTabStripModel()->GetTabGroupForTab(i).has_value()) {
        t.Set("group_id",
              bwi->GetTabStripModel()->GetTabGroupForTab(i)->ToString());
      }
      tabs.Append(std::move(t));
    }
    w.Set("tabs", std::move(tabs));

    base::DictValue groups;
    for (const auto& group_id :
         bwi->GetTabStripModel()->group_model()->ListTabGroups()) {
      const TabGroup* group =
          bwi->GetTabStripModel()->group_model()->GetTabGroup(group_id);
      base::DictValue g;
      g.Set("title", group->visual_data()->title());
      groups.Set(group_id.ToString(), std::move(g));
    }
    w.Set("groups", std::move(groups));

    expected_windows.Append(std::move(w));
  }
  expected.Set("windows", std::move(expected_windows));
  return expected;
}

void ExpectOutputMatchesWindowSkeleton(base::Location location,
                                       const std::string& out_json,
                                       Profile* profile) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  base::Value out_val = base::test::ParseJson(out_json);
  EXPECT_THAT(out_val, base::test::IsSupersetOfValue(
                           BuildExpectedOutputSubset(profile)));
}

}  // namespace

class TabManagementToolBrowserTest : public InProcessBrowserTest {
 public:
  TabManagementToolBrowserTest() = default;

  Profile* profile() { return browser()->profile(); }

  // Add a tab with given URL to the specified browser, return its tab handle.
  int AddTabAndGetHandle(Browser* b, const GURL& url, bool foreground = true) {
    TabStripModel* strip = b->tab_strip_model();
    int before_count = strip->count();
    chrome::AddTabAt(b, url, -1, foreground);
    int new_index = strip->count() - 1;
    if (strip->count() == before_count) {
      // Fallback: if nothing changed, return active tab handle.
      content::WebContents* active = strip->GetActiveWebContents();
      return GetTabHandle(active);
    }
    content::WebContents* contents = strip->GetWebContentsAt(new_index);
    return GetTabHandle(contents);
  }
};

IN_PROC_BROWSER_TEST_F(TabManagementToolBrowserTest, TabManagementToolTest) {
  // Single comprehensive browser test that covers actual inter-browser
  // and tab strip operations. Any scenarios which test other behavior
  // of the Tool, such as pre-operation validation, should be unit tested.
  TabManagementTool tool(profile());

  tool.UserPermissionGranted("");

  // Setup: create tabs across two windows
  Browser* b1 = browser();
  int initial_b1_count = b1->tab_strip_model()->count();
  AddTabAndGetHandle(b1, GURL("https://a.test/"));
  AddTabAndGetHandle(b1, GURL("https://b.test/"));

  Browser* b2 = CreateBrowser(profile());
  int initial_b2_count = b2->tab_strip_model()->count();
  AddTabAndGetHandle(b2, GURL("https://c.test/"));
  AddTabAndGetHandle(b2, GURL("https://d.test/"));

  // Sanity: the counts include the newly added tabs without waiting
  ASSERT_EQ(b1->tab_strip_model()->count(), initial_b1_count + 2);
  ASSERT_EQ(b2->tab_strip_model()->count(), initial_b2_count + 2);

  // List action should return structured JSON of windows and tabs, matching the
  // current profile window set.
  {
    std::string response =
        RunToolAndGetText(FROM_HERE, &tool, R"JSON({"action":"list"})JSON");

    ExpectOutputMatchesWindowSkeleton(FROM_HERE, response, profile());

    // Sanity: we have both windows
    auto list_root = base::test::ParseJsonDict(response);
    const base::ListValue* output_windows = list_root.FindList("windows");
    ASSERT_TRUE(output_windows);
    ASSERT_EQ(output_windows->size(), 2u);
  }
}

}  // namespace ai_chat
