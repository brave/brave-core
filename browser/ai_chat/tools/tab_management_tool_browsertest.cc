// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/tab_management_tool.h"

#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "base/functional/callback_helpers.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
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
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/components/local_ai/core/pref_names.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "components/prefs/pref_service.h"
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)

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

const SessionID& GetSessionIdForTabId(int tab_id) {
  return tabs::TabHandle(tab_id)
      .Get()
      ->GetBrowserWindowInterface()
      ->GetSessionID();
}

std::optional<tab_groups::TabGroupId> GetGroupIdForTabId(int tab_id) {
  return tabs::TabHandle(tab_id).Get()->GetGroup();
}

TabGroup* GetGroupForTabId(int tab_id) {
  auto* tab = tabs::TabHandle(tab_id).Get();
  auto* tab_strip = tab->GetBrowserWindowInterface()->GetTabStripModel();
  auto group_id = tab_strip->GetTabGroupForTab(
      tab_strip->GetIndexOfWebContents(tab->GetContents()));
  if (!group_id.has_value()) {
    return nullptr;
  }
  return tab_strip->group_model()->GetTabGroup(group_id.value());
}

BrowserWindowInterface* FindBrowserBySessionId(Profile* profile,
                                               int session_id) {
  for (BrowserWindowInterface* bwi : GetAllBrowserWindowInterfaces()) {
    if (bwi->GetProfile() == profile &&
        bwi->GetSessionID().id() == session_id) {
      return bwi;
    }
  }
  return nullptr;
}

size_t GetBrowserCount() {
  size_t count = 0;
  for (BrowserWindowInterface* _ : GetAllBrowserWindowInterfaces()) {
    ++count;
  }
  return count;
}

size_t GetTabCount(Profile* profile) {
  size_t count = 0;
  for (BrowserWindowInterface* bwi : GetAllBrowserWindowInterfaces()) {
    if (bwi->GetProfile() != profile) {
      continue;
    }
    count += bwi->GetTabStripModel()->count();
  }
  return count;
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

const base::DictValue* FindWindowInOutput(const base::DictValue& root,
                                          int window_id) {
  const base::ListValue* windows = root.FindList("windows");
  if (!windows) {
    return nullptr;
  }
  for (const auto& w : *windows) {
    const base::DictValue& wdict = w.GetDict();
    std::optional<int> wid = wdict.FindInt("window_id");
    if (wid && *wid == window_id) {
      return &wdict;
    }
  }
  return nullptr;
}

bool GetGroupVisualsFromWindowOutput(const base::DictValue& window_dict,
                                     const std::string& group_id,
                                     std::string* out_title,
                                     std::string* out_color) {
  const base::DictValue* groups = window_dict.FindDict("groups");
  if (!groups) {
    return false;
  }
  const base::DictValue* g = groups->FindDict(group_id);
  if (!g) {
    return false;
  }
  if (out_title) {
    const std::string* title = g->FindString("title");
    *out_title = title ? *title : std::string();
  }
  if (out_color) {
    const std::string* color = g->FindString("color");
    *out_color = color ? *color : std::string();
  }
  return true;
}

void ExpectOutputMatchesWindowSkeleton(base::Location location,
                                       const std::string& out_json,
                                       Profile* profile) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  base::Value out_val = base::test::ParseJson(out_json);
  EXPECT_THAT(out_val, base::test::IsSupersetOfValue(
                           BuildExpectedOutputSubset(profile)));
}

void ExpectGroupVisualsInWindowOutput(
    base::Location location,
    const base::DictValue& root,
    int window_id,
    const std::string& group_id,
    std::optional<std::string> expected_title,
    std::optional<std::string> expected_color) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  const base::DictValue* w = FindWindowInOutput(root, window_id);
  EXPECT_TRUE(w);
  std::string title, color;
  EXPECT_TRUE(GetGroupVisualsFromWindowOutput(*w, group_id, &title, &color));
  if (expected_title) {
    EXPECT_EQ(title, *expected_title);
  }
  if (expected_color) {
    EXPECT_EQ(color, *expected_color);
  }
}

}  // namespace

class TabManagementToolBrowserTest : public InProcessBrowserTest {
 public:
  TabManagementToolBrowserTest() = default;

  Profile* profile() { return browser()->GetProfile(); }

  // Add a tab with given URL to the specified browser, return its tab handle.
  int AddTabAndGetHandle(BrowserWindowInterface* b,
                         const GURL& url,
                         bool foreground = true) {
    TabStripModel* strip = b->GetTabStripModel();
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

  tool.UserPermissionGranted("", *mojom::PermissionChallenge::New());

  // Setup: create tabs across two windows
  BrowserWindowInterface* b1 = browser();
  int initial_b1_count = b1->tab_strip_model()->count();
  AddTabAndGetHandle(b1, GURL("https://a.test/"));
  AddTabAndGetHandle(b1, GURL("https://b.test/"));

  BrowserWindowInterface* b2 = CreateBrowser(profile());
  int initial_b2_count = b2->tab_strip_model()->count();
  AddTabAndGetHandle(b2, GURL("https://c.test/"));
  AddTabAndGetHandle(b2, GURL("https://d.test/"));

  // Tabs we will be manipulating
  int a_handle = AddTabAndGetHandle(b1, GURL("https://one.test/"));
  int b_handle = AddTabAndGetHandle(b1, GURL("https://two.test/"));

  // Sanity: the counts include the newly added tabs without waiting
  ASSERT_EQ(b1->tab_strip_model()->count(), initial_b1_count + 4);
  ASSERT_EQ(b2->tab_strip_model()->count(), initial_b2_count + 2);

  // List action should return structured JSON of windows and tabs, matching the
  // current profile window set.
  // First operation, so grant permission
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

  // Test error cases and edge conditions that depend on real browsers

  // Test moving a tab that doesn't exist
  std::string response_nonexistent_tab = RunToolAndGetText(FROM_HERE, &tool,
                                                           R"JSON({
        "action": "move_tabs",
        "tab_ids": [99999, 88888],
        "window_id": -1
      })JSON");
  EXPECT_THAT(response_nonexistent_tab,
              testing::HasSubstr("No valid tabs found to move"));

  // Test moving an entire group that doesn't exist
  std::string response_move_bad_group = RunToolAndGetText(FROM_HERE, &tool,
                                                          R"JSON({
        "action": "move_group",
        "group_id": "totally-bogus-group-id",
        "window_id": -1
      })JSON");
  EXPECT_THAT(response_move_bad_group, testing::HasSubstr("Group not found"));

  // Test trying to move to an invalid window ID
  std::string response_invalid_window = RunToolAndGetText(FROM_HERE, &tool,
                                                          absl::StrFormat(
                                                              R"JSON({
        "action": "move_tabs",
        "tab_ids": [%d],
        "window_id": 999999
      })JSON",
                                                              b_handle));
  EXPECT_THAT(response_invalid_window,
              testing::HasSubstr("Target window not found"));

  // Invalid window_id (negative but not -1)
  EXPECT_THAT(RunToolAndGetText(
                  FROM_HERE, &tool,
                  absl::StrFormat(
                      R"({"action":"move_tabs","tab_ids":[%d],"window_id":-5})",
                      b_handle)),
              testing::HasSubstr("Invalid window ID"));

  // Very large window_id
  EXPECT_THAT(
      RunToolAndGetText(
          FROM_HERE, &tool,
          absl::StrFormat(
              R"({"action":"move_tabs","tab_ids":[%d],"window_id":999999})",
              b_handle)),
      testing::HasSubstr("Target window not found"));

  // Group creation, management and movement

  // Create a group
  {
    std::string create_response = RunToolAndGetText(FROM_HERE, &tool,
                                                    absl::StrFormat(
                                                        R"JSON({
        "action": "create_group",
        "tab_ids": [%d],
        "group_title": "TestGroup"
      })JSON",
                                                        a_handle));
    // Verify the tab is grouped and the returned group id is valid
    base::DictValue create_dict = base::test::ParseJsonDict(create_response);
    const std::string* created_group_id =
        create_dict.FindString("created_group_id");
    ASSERT_TRUE(created_group_id);
    ASSERT_FALSE(created_group_id->empty());
    auto* tab = tabs::TabHandle(a_handle).Get();
    ASSERT_TRUE(tab);
    EXPECT_TRUE(tab->GetGroup().has_value());
    EXPECT_EQ(tab->GetGroup().value().ToString(), *created_group_id);
    ExpectOutputMatchesWindowSkeleton(FROM_HERE, create_response, profile());
  }

  // Remove a single tab from the test group
  {
    ASSERT_TRUE(tabs::TabHandle(a_handle).Get()->GetGroup().has_value());
    std::string remove_response = RunToolAndGetText(FROM_HERE, &tool,
                                                    absl::StrFormat(
                                                        R"JSON({
        "action": "remove_from_group",
        "tab_ids": [%d]
      })JSON",
                                                        a_handle));

    // Verify it's not in a group anymore
    EXPECT_FALSE(tabs::TabHandle(a_handle).Get()->GetGroup().has_value());

    // Verify the group is not in the output
    ExpectOutputMatchesWindowSkeleton(FROM_HERE, remove_response, profile());
  }

  // Create a group with tabs from different windows and specify visuals
  // The tabs should be moved to the first tab's window and grouped together
  int e_handle = AddTabAndGetHandle(b2, GURL("https://example.test/"));
  int f_handle = AddTabAndGetHandle(b2, GURL("https://example2.test/"));
  {
    auto first_window_id = GetSessionIdForTabId(a_handle);
    ASSERT_EQ(GetSessionIdForTabId(a_handle), GetSessionIdForTabId(b_handle));
    ASSERT_NE(GetSessionIdForTabId(a_handle), GetSessionIdForTabId(e_handle));
    ASSERT_NE(GetSessionIdForTabId(a_handle), GetSessionIdForTabId(f_handle));

    std::string response =
        RunToolAndGetText(FROM_HERE, &tool,
                          absl::StrFormat(
                              R"JSON({
        "action": "create_group",
        "tab_ids": [%d, %d, %d, %d],
        "group_title": "G1",
        "group_color": "blue"
      })JSON",
                              a_handle, b_handle, e_handle, f_handle));

    base::DictValue response_dict = base::test::ParseJsonDict(response);
    // Validate created_group_id present
    const std::string* created_group_id =
        response_dict.FindString("created_group_id");
    ASSERT_TRUE(created_group_id);
    ASSERT_FALSE(created_group_id->empty());

    ExpectGroupVisualsInWindowOutput(FROM_HERE, response_dict,
                                     GetSessionIdForTabId(a_handle).id(),
                                     *created_group_id, "G1", "blue");

    // Expect tabs to all actually be in the same window
    // and that everything moved to the first TabID's window
    EXPECT_EQ(GetSessionIdForTabId(a_handle), GetSessionIdForTabId(b_handle));
    EXPECT_EQ(GetSessionIdForTabId(a_handle), GetSessionIdForTabId(e_handle));
    EXPECT_EQ(GetSessionIdForTabId(a_handle), GetSessionIdForTabId(f_handle));
    ASSERT_EQ(GetSessionIdForTabId(a_handle), first_window_id);
    // and group
    EXPECT_EQ(GetGroupIdForTabId(a_handle), GetGroupIdForTabId(b_handle));
    EXPECT_EQ(GetGroupIdForTabId(a_handle), GetGroupIdForTabId(e_handle));
    EXPECT_EQ(GetGroupIdForTabId(a_handle), GetGroupIdForTabId(f_handle));

    // Verify tool output matches current state
    ExpectOutputMatchesWindowSkeleton(FROM_HERE, response, profile());
  }

  // Moving tab to specific group in the same window
  {
    int tab_to_move = AddTabAndGetHandle(b1, GURL("https://move.test/"));
    ASSERT_FALSE(GetGroupIdForTabId(tab_to_move).has_value());
    ASSERT_EQ(GetSessionIdForTabId(tab_to_move),
              GetSessionIdForTabId(a_handle));
    auto group_id = GetGroupIdForTabId(a_handle);
    ASSERT_TRUE(group_id.has_value());
    std::string response =
        RunToolAndGetText(FROM_HERE, &tool,
                          absl::StrFormat(
                              R"JSON({
          "action": "move_tabs",
          "tab_ids": [%d],
          "destination_group_id": "%s"
        })JSON",
                              tab_to_move, group_id.value().ToString()));

    // Verify tab is now part of the group
    ASSERT_TRUE(GetGroupIdForTabId(tab_to_move).has_value());
    ASSERT_EQ(GetGroupIdForTabId(tab_to_move).value(), group_id.value());
    ExpectOutputMatchesWindowSkeleton(FROM_HERE, response, profile());
  }

  // Moving tab to a group in a different window
  {
    int tab_to_move = AddTabAndGetHandle(b2, GURL("https://move-window.test/"));
    BrowserWindow::FromBrowser(b2)->Activate();
    ASSERT_TRUE(b2->IsActive());
    b2->tab_strip_model()->ActivateTabAt(b2->GetTabStripModel()->GetIndexOfTab(
        tabs::TabHandle(tab_to_move).Get()));
    ASSERT_TRUE(b2->IsActive());
    ASSERT_TRUE(tabs::TabHandle(tab_to_move).Get()->IsActivated());
    ASSERT_EQ(GetSessionIdForTabId(tab_to_move), b2->GetSessionID());
    // Get a group in a different window
    ASSERT_EQ(GetSessionIdForTabId(b_handle), b1->GetSessionID());
    auto group_id = GetGroupIdForTabId(b_handle);
    ASSERT_TRUE(group_id.has_value());
    ASSERT_FALSE(GetGroupIdForTabId(tab_to_move).has_value());
    ASSERT_NE(GetSessionIdForTabId(tab_to_move),
              GetSessionIdForTabId(b_handle));
    std::string response =
        RunToolAndGetText(FROM_HERE, &tool,
                          absl::StrFormat(
                              R"JSON({
        "action": "move_tabs",
        "tab_ids": [%d],
        "destination_group_id": "%s"
      })JSON",
                              tab_to_move, group_id.value().ToString()));

    base::DictValue response_dict = base::test::ParseJsonDict(response);

    // Verify the tab moved to the target window and joined the group
    ASSERT_EQ(GetSessionIdForTabId(tab_to_move),
              GetSessionIdForTabId(b_handle));
    ASSERT_EQ(GetGroupIdForTabId(tab_to_move).value(), group_id.value());
// Checking windows' activation state is flaky in browser tests.
#if !BUILDFLAG(IS_MAC)
    // Verify new window is active and tab is active in the window
    EXPECT_TRUE(
        BrowserWindowInterface::FromSessionID(GetSessionIdForTabId(tab_to_move))
            ->IsActive());
    EXPECT_FALSE(b2->IsActive());
    EXPECT_TRUE(tabs::TabHandle(tab_to_move).Get()->IsActivated());
#endif
    ExpectOutputMatchesWindowSkeleton(FROM_HERE, response, profile());
  }

  // Moving an entire group to a new window
  {
    auto* group = GetGroupForTabId(a_handle);
    ASSERT_TRUE(group);
    // There are now 6 tabs in the group - the 4 from the initial creation and
    // we've done 2 moves since
    EXPECT_EQ(group->tab_count(), 6);
    auto original_group_id = group->id();
    auto original_window_id = GetSessionIdForTabId(a_handle);
    size_t original_browser_count = GetBrowserCount();
    std::string response = RunToolAndGetText(FROM_HERE, &tool,
                                             absl::StrFormat(
                                                 R"JSON({
        "action": "move_group",
        "group_id": "%s",
        "window_id": -1
      })JSON",
                                                 group->id().ToString()));

    // Verify a new window was created and the group was moved to it
    base::DictValue response_dict = base::test::ParseJsonDict(response);
    std::optional<int> new_window_id = response_dict.FindInt("new_window_id");
    ASSERT_TRUE(new_window_id);
    ASSERT_NE(original_window_id.id(), *new_window_id);
    // New window should have been created.
    ASSERT_EQ(GetBrowserCount(), original_browser_count + 1u);
    BrowserWindowInterface* moved_browser =
        FindBrowserBySessionId(profile(), *new_window_id);
    ASSERT_TRUE(moved_browser);
    // Verify the group exists in the new window with same title/color, and tabs
    // are listed under that window in the tool response.
    EXPECT_EQ(GetSessionIdForTabId(a_handle).id(), *new_window_id);
    group = GetGroupForTabId(a_handle);
    ASSERT_TRUE(group);
    EXPECT_EQ(group->tab_count(), 6);
    // When moving a group entirely, the group ID stays the same.
    EXPECT_EQ(group->id().ToString(), original_group_id.ToString());
    const auto* vd = group->visual_data();
    EXPECT_EQ(vd->title(), u"G1");
    EXPECT_EQ(vd->color(), tab_groups::TabGroupColorId::kBlue);

    // Validate tool JSON: all tabs appear under new_window_id and have same
    // group id
    ExpectOutputMatchesWindowSkeleton(FROM_HERE, response, profile());
    // visuals match
    ExpectGroupVisualsInWindowOutput(FROM_HERE, response_dict, *new_window_id,
                                     group->id().ToString(), "G1", "blue");
  }

  // Add some extra tabs to the new window so the group has room to move
  BrowserWindowInterface* moved_browser =
      FindBrowserBySessionId(profile(), GetSessionIdForTabId(a_handle).id());
  AddTabAndGetHandle(moved_browser, GURL("https://extra1.test/"));
  AddTabAndGetHandle(moved_browser, GURL("https://extra2.test/"));

  // Moving group within the same window
  {
    auto* group = GetGroupForTabId(a_handle);
    ASSERT_TRUE(group);
    auto group_id = group->id();
    TabStripModel* target_strip = moved_browser->GetTabStripModel();
    int original_index = target_strip->GetIndexOfWebContents(
        tabs::TabHandle(a_handle).Get()->GetContents());

    std::string response =
        RunToolAndGetText(FROM_HERE, &tool,
                          absl::StrFormat(
                              R"JSON({
        "action": "move_group",
        "group_id": "%s",
        "index": %d
      })JSON",
                              group_id.ToString(),
                              target_strip->count() - 1));  // Move to end

    base::DictValue response_dict = base::test::ParseJsonDict(response);
    int new_index = target_strip->GetIndexOfWebContents(
        tabs::TabHandle(a_handle).Get()->GetContents());
    EXPECT_GT(new_index, original_index);  // Should be later in tab strip

    // Test moving group to same position (should be no-op)
    int current_index = target_strip->GetIndexOfWebContents(
        tabs::TabHandle(a_handle).Get()->GetContents());
    response =
        RunToolAndGetText(FROM_HERE, &tool,
                          absl::StrFormat(
                              R"JSON({
        "action": "move_group",
        "group_id": "%s",
        "index": %d
      })JSON",
                              group_id.ToString(),
                              current_index));  // Move to current position
    EXPECT_THAT(response,
                testing::HasSubstr("Group already at target position"));
    // Verify the group is still in the same position
    int same_index = target_strip->GetIndexOfWebContents(
        tabs::TabHandle(a_handle).Get()->GetContents());
    EXPECT_EQ(same_index, current_index);
  }

  // Test group visual updates

  // Update group visuals: change title and color.
  {
    auto* group = GetGroupForTabId(a_handle);
    ASSERT_TRUE(group);
    ASSERT_NE(group->visual_data()->title(), u"G1B");
    ASSERT_NE(group->visual_data()->color(), tab_groups::TabGroupColorId::kRed);
    auto group_id = group->id();
    std::string response = RunToolAndGetText(FROM_HERE, &tool,
                                             absl::StrFormat(
                                                 R"JSON({
        "action": "update_group",
        "group_id": "%s",
        "group_title": "G1B",
        "group_color": "red"
      })JSON",
                                                 group_id.ToString()));

    base::DictValue response_dict = base::test::ParseJsonDict(response);
    EXPECT_THAT(response_dict,
                base::test::IsSupersetOfValue(base::test::ParseJson(
                    R"JSON({"message":"Successfully updated group"})JSON")));

    // Verify the group visuals were updated
    EXPECT_EQ(group->visual_data()->title(), u"G1B");
    EXPECT_EQ(group->visual_data()->color(), tab_groups::TabGroupColorId::kRed);
    ExpectGroupVisualsInWindowOutput(FROM_HERE, response_dict,
                                     GetSessionIdForTabId(a_handle).id(),
                                     group_id.ToString(), "G1B", "red");
  }

  // Try to remove a tab from a group when it's not in any group - should
  // succeed
  {
    int tab_to_remove = AddTabAndGetHandle(b1, GURL("https://remove.test/"));
    ASSERT_FALSE(GetGroupIdForTabId(tab_to_remove).has_value());
    std::string response = RunToolAndGetText(FROM_HERE, &tool,
                                             absl::StrFormat(
                                                 R"JSON({
          "action": "remove_from_group",
          "tab_ids": [%d]
        })JSON",
                                                 tab_to_remove));
    EXPECT_THAT(response, testing::HasSubstr(
                              "No valid tabs found to remove from groups"));
  }

  // Move tabs from different windows without specifying a group
  // or window destination. This should move all tabs to the window of the first
  // tab, and ungroup.
  int tab_from_b1 = AddTabAndGetHandle(b1, GURL("https://cross1.test/"));
  int tab_from_b2 = AddTabAndGetHandle(b2, GURL("https://cross2.test/"));
  int another_tab_b1 = AddTabAndGetHandle(b1, GURL("https://another1.test/"));
  int another_tab_b2 = AddTabAndGetHandle(b2, GURL("https://another2.test/"));
  {
    // Add the tab to a group to verify what happens when a target is inferred
    // from the first tab id
    tab_groups::TabGroupId group_id = b1->GetTabStripModel()->AddToNewGroup(
        {b1->GetTabStripModel()->GetIndexOfWebContents(
            tabs::TabHandle(tab_from_b1).Get()->GetContents())});
    ASSERT_FALSE(group_id.is_empty());
    ASSERT_EQ(GetGroupIdForTabId(tab_from_b1), group_id);
    std::string response = RunToolAndGetText(
        FROM_HERE, &tool,
        absl::StrFormat(
            R"JSON({
          "action": "move_tabs",
          "tab_ids": [%d, %d, %d, %d]
        })JSON",
            tab_from_b1, another_tab_b1, tab_from_b2, another_tab_b2));

    // All tabs should move to first tab's window (b1), and if the group exists
    // there, join it
    base::DictValue response_dict = base::test::ParseJsonDict(response);
    EXPECT_EQ(GetSessionIdForTabId(tab_from_b1), b1->GetSessionID());
    EXPECT_EQ(GetSessionIdForTabId(tab_from_b1),
              GetSessionIdForTabId(tab_from_b2));
    EXPECT_EQ(GetSessionIdForTabId(tab_from_b1),
              GetSessionIdForTabId(another_tab_b2));
    EXPECT_EQ(GetSessionIdForTabId(tab_from_b1),
              GetSessionIdForTabId(another_tab_b1));
    // Moved tabs are not grouped
    EXPECT_FALSE(GetGroupIdForTabId(tab_from_b1).has_value());
    EXPECT_FALSE(GetGroupIdForTabId(tab_from_b2).has_value());
    EXPECT_FALSE(GetGroupIdForTabId(another_tab_b1).has_value());
    EXPECT_FALSE(GetGroupIdForTabId(another_tab_b2).has_value());
    ExpectOutputMatchesWindowSkeleton(FROM_HERE, response, profile());
  }

  auto last_browser_count = GetBrowserCount();

  // Moving all tabs from a window should result in the window
  // being closed, and not returning a window with an empty tab strip.
  // This validates the timing of the result in such scenarios.
  {
    BrowserWindowInterface* bnew = CreateBrowser(profile());
    auto bnew_session_id = bnew->GetSessionID();
    auto bnewa =
        bnew->tab_strip_model()->GetTabAtIndex(0)->GetHandle().raw_value();
    auto bnewb = AddTabAndGetHandle(bnew, GURL("https://move-all.test/"));
    auto bnewc = AddTabAndGetHandle(bnew, GURL("https://move-all.test/"));
    ASSERT_EQ(GetBrowserCount(), last_browser_count + 1);
    last_browser_count = GetBrowserCount();
    std::string response = RunToolAndGetText(FROM_HERE, &tool,
                                             absl::StrFormat(
                                                 R"JSON({
        "action": "move_tabs",
        "tab_ids": [%d, %d, %d],
        "window_id": -1
      })JSON",
                                                 bnewa, bnewb, bnewc));
    // 1 browser added, 1 browser removed
    EXPECT_EQ(GetBrowserCount(), last_browser_count);
    EXPECT_NE(GetSessionIdForTabId(bnewa), bnew_session_id);
    EXPECT_NE(GetSessionIdForTabId(bnewb), bnew_session_id);
    EXPECT_NE(GetSessionIdForTabId(bnewc), bnew_session_id);
    ExpectOutputMatchesWindowSkeleton(FROM_HERE, response, profile());
  }

  // Similarly, moving all groups from a window should result in the window
  // being closed and not returning a window with an empty tab strip.
  {
    BrowserWindowInterface* bnew = CreateBrowser(profile());
    auto bnew_session_id = bnew->GetSessionID();
    auto bnewa =
        bnew->tab_strip_model()->GetTabAtIndex(0)->GetHandle().raw_value();
    auto bnewb = AddTabAndGetHandle(bnew, GURL("https://move-all.test/"));
    auto bnewc = AddTabAndGetHandle(bnew, GURL("https://move-all.test/"));
    ASSERT_EQ(GetBrowserCount(), last_browser_count + 1);
    last_browser_count = GetBrowserCount();
    auto group_id = bnew->tab_strip_model()->AddToNewGroup({0, 1, 2});
    ASSERT_FALSE(group_id.is_empty());
    ASSERT_EQ(bnew->tab_strip_model()->GetTabGroupForTab(0), group_id);
    ASSERT_EQ(bnew->tab_strip_model()->GetTabGroupForTab(1), group_id);
    ASSERT_EQ(bnew->tab_strip_model()->GetTabGroupForTab(2), group_id);

    std::string response = RunToolAndGetText(FROM_HERE, &tool,
                                             absl::StrFormat(
                                                 R"JSON({
        "action": "move_group",
        "group_id": "%s",
        "window_id": -1
      })JSON",
                                                 group_id.ToString()));
    // 1 browser added, 1 browser removed
    EXPECT_EQ(GetBrowserCount(), last_browser_count);
    EXPECT_NE(GetSessionIdForTabId(bnewa), bnew_session_id);
    EXPECT_NE(GetSessionIdForTabId(bnewb), bnew_session_id);
    EXPECT_NE(GetSessionIdForTabId(bnewc), bnew_session_id);
    ExpectOutputMatchesWindowSkeleton(FROM_HERE, response, profile());
  }
}

// Moving multiple tabs in a single request must preserve the request order of
// the tabs at the destination, both within a window and across windows.
IN_PROC_BROWSER_TEST_F(TabManagementToolBrowserTest,
                       MoveMultipleTabsPreservesOrder) {
  TabManagementTool tool(profile());
  tool.UserPermissionGranted("", *mojom::PermissionChallenge::New());

  BrowserWindowInterface* b1 = browser();
  TabStripModel* strip1 = b1->tab_strip_model();

  int t1 = AddTabAndGetHandle(b1, GURL("https://order1.test/"));
  int t2 = AddTabAndGetHandle(b1, GURL("https://order2.test/"));
  int t3 = AddTabAndGetHandle(b1, GURL("https://order3.test/"));

  auto index_of = [](int tab_id) {
    auto* tab = tabs::TabHandle(tab_id).Get();
    return tab->GetBrowserWindowInterface()
        ->GetTabStripModel()
        ->GetIndexOfWebContents(tab->GetContents());
  };

  // Same-window move: request order [t3, t1, t2]. They must land contiguously
  // in that order rather than reversed.
  {
    RunToolAndGetText(FROM_HERE, &tool,
                      absl::StrFormat(
                          R"JSON({
        "action": "move_tabs",
        "tab_ids": [%d, %d, %d],
        "index": %d
      })JSON",
                          t3, t1, t2, strip1->count() - 1));

    int i3 = index_of(t3);
    EXPECT_EQ(index_of(t1), i3 + 1);
    EXPECT_EQ(index_of(t2), i3 + 2);
  }

  // Cross-window move: move the same tabs into a second window in request order
  // [t2, t3, t1].
  {
    BrowserWindowInterface* b2 = CreateBrowser(profile());
    AddTabAndGetHandle(b2, GURL("https://other.test/"));
    int target_window_id = b2->GetSessionID().id();

    RunToolAndGetText(FROM_HERE, &tool,
                      absl::StrFormat(
                          R"JSON({
        "action": "move_tabs",
        "tab_ids": [%d, %d, %d],
        "window_id": %d
      })JSON",
                          t2, t3, t1, target_window_id));

    ASSERT_EQ(GetSessionIdForTabId(t1).id(), target_window_id);
    ASSERT_EQ(GetSessionIdForTabId(t2).id(), target_window_id);
    ASSERT_EQ(GetSessionIdForTabId(t3).id(), target_window_id);

    int i2 = index_of(t2);
    EXPECT_EQ(index_of(t3), i2 + 1);
    EXPECT_EQ(index_of(t1), i2 + 2);
  }
}

IN_PROC_BROWSER_TEST_F(TabManagementToolBrowserTest, CloseTabsAcrossWindows) {
  TabManagementTool tool(profile());
  tool.UserPermissionGranted("", *mojom::PermissionChallenge::New());

  BrowserWindowInterface* b1 = browser();
  BrowserWindowInterface* b2 = CreateBrowser(profile());

  int t1 = AddTabAndGetHandle(b1, GURL("https://close1.test/"));
  int t2 = AddTabAndGetHandle(b2, GURL("https://close2.test/"));
  // A tab we keep open, to confirm only the requested tabs are closed.
  int keep = AddTabAndGetHandle(b1, GURL("https://keep.test/"));

  ASSERT_NE(GetSessionIdForTabId(t1), GetSessionIdForTabId(t2));
  const size_t initial_tab_count = GetTabCount(profile());

  std::string response = RunToolAndGetText(FROM_HERE, &tool,
                                           absl::StrFormat(
                                               R"JSON({
      "action": "close",
      "tab_ids": [%d, %d]
    })JSON",
                                               t1, t2));

  base::DictValue response_dict = base::test::ParseJsonDict(response);
  EXPECT_THAT(response_dict,
              base::test::IsSupersetOfValue(base::test::ParseJson(
                  R"JSON({"message":"Successfully closed 2 tab(s)"})JSON")));

  // The waiter must not report until the tabs are actually destroyed.
  EXPECT_FALSE(tabs::TabHandle(t1).Get());
  EXPECT_FALSE(tabs::TabHandle(t2).Get());
  EXPECT_TRUE(tabs::TabHandle(keep).Get());
  EXPECT_EQ(GetTabCount(profile()), initial_tab_count - 2);
  ExpectOutputMatchesWindowSkeleton(FROM_HERE, response, profile());
}

#if BUILDFLAG(ENABLE_LOCAL_AI)

// Page excerpts come from the history embeddings index, so these tests turn
// on everything `IsHistoryEmbeddingsEnabledForProfile` needs.
class TabManagementToolPageContentBrowserTest
    : public TabManagementToolBrowserTest {
 public:
  TabManagementToolPageContentBrowserTest() {
    scoped_features_.InitAndEnableFeature(
        history_embeddings::kHistoryEmbeddings);
  }

  void SetUpOnMainThread() override {
    TabManagementToolBrowserTest::SetUpOnMainThread();
    SetSemanticHistorySearchEnabled(true);
  }

  void SetSemanticHistorySearchEnabled(bool enabled) {
    profile()->GetPrefs()->SetBoolean(
        local_ai::prefs::kBraveHistoryEmbeddingsEnabled, enabled);
  }

  // Stands in for the embeddings database, which needs a downloaded model to
  // hold anything. Every URL gets `passage`; `on_fetch` runs before replying.
  void SetPassage(TabManagementTool& tool,
                  const std::string& passage,
                  base::RepeatingClosure on_fetch = base::DoNothing()) {
    tool.SetTabPassagesFetcherForTesting(base::BindLambdaForTesting(
        [passage, on_fetch](
            const std::vector<GURL>& urls,
            base::OnceCallback<void(std::vector<std::vector<std::string>>)>
                callback) {
          on_fetch.Run();
          std::move(callback).Run(
              std::vector<std::vector<std::string>>(urls.size(), {passage}));
        }));
  }

  mojom::ToolUseEventPtr ToolUse(const std::string& id,
                                 const std::string& input_json) {
    return mojom::ToolUseEvent::New(mojom::kTabManagementToolName, id,
                                    input_json, std::nullopt, std::nullopt,
                                    nullptr, false);
  }

  mojom::PermissionChallengePtr GetChallenge(TabManagementTool& tool,
                                             const mojom::ToolUseEvent& use) {
    auto result = tool.RequiresUserInteractionBeforeHandling(use);
    if (!std::holds_alternative<mojom::PermissionChallengePtr>(result)) {
      return nullptr;
    }
    return std::move(std::get<mojom::PermissionChallengePtr>(result));
  }

  // Passages of the tab at `url` in a list result, or null if it has none.
  const base::ListValue* FindPassages(const base::DictValue& output,
                                      const GURL& url) {
    for (const auto& window : *output.FindList("windows")) {
      for (const auto& tab : *window.GetDict().FindList("tabs")) {
        if (*tab.GetDict().FindString("url") == url.spec()) {
          return tab.GetDict().FindList("passages");
        }
      }
    }
    return nullptr;
  }

  static constexpr char kListInput[] =
      R"({"action":"list", "plan":"group by topic"})";
  static constexpr char kPassage[] = "Lighthouses and lightships";

 private:
  base::test::ScopedFeatureList scoped_features_;
};

// When page content is readable, the one challenge a conversation starts with
// says so, and answering it sends the excerpts.
IN_PROC_BROWSER_TEST_F(TabManagementToolPageContentBrowserTest,
                       CombinedChallengeSendsPassages) {
  TabManagementTool tool(profile());
  SetPassage(tool, kPassage);
  const GURL url("https://a.test/");
  AddTabAndGetHandle(browser(), url);

  auto use = ToolUse("1", kListInput);
  auto challenge = GetChallenge(tool, *use);
  ASSERT_TRUE(challenge);
  EXPECT_EQ(challenge->plan, "group by topic");
  // The summary stays the tool's usual one; only what allowing it sends grows.
  EXPECT_FALSE(challenge->description);
  EXPECT_THAT(challenge->implications,
              testing::Optional(testing::HasSubstr("page text")));

  tool.UserPermissionGranted(use->id, *challenge);
  EXPECT_FALSE(GetChallenge(tool, *ToolUse("2", kListInput)));

  base::DictValue output = base::test::ParseJsonDict(
      RunToolAndGetText(FROM_HERE, &tool, kListInput));
  const base::ListValue* passages = FindPassages(output, url);
  ASSERT_TRUE(passages);
  EXPECT_EQ(*passages, base::ListValue().Append(kPassage));
}

// Without the index, the challenge only covers tabs and no excerpts are read,
// even if the index turns up later in the conversation.
IN_PROC_BROWSER_TEST_F(TabManagementToolPageContentBrowserTest,
                       NoPassagesWithoutSemanticHistorySearch) {
  SetSemanticHistorySearchEnabled(false);
  TabManagementTool tool(profile());
  bool fetched = false;
  SetPassage(tool, kPassage,
             base::BindLambdaForTesting([&] { fetched = true; }));
  const GURL url("https://a.test/");
  AddTabAndGetHandle(browser(), url);

  auto use = ToolUse("1", kListInput);
  auto challenge = GetChallenge(tool, *use);
  ASSERT_TRUE(challenge);
  EXPECT_FALSE(challenge->implications);
  tool.UserPermissionGranted(use->id, *challenge);

  base::DictValue output = base::test::ParseJsonDict(
      RunToolAndGetText(FROM_HERE, &tool, kListInput));
  EXPECT_FALSE(FindPassages(output, url));
  EXPECT_FALSE(fetched);

  // Tab access was granted without page content, so turning the index on asks
  // again, and only for a list, which is the only action sending excerpts.
  SetSemanticHistorySearchEnabled(true);
  EXPECT_FALSE(GetChallenge(
      tool, *ToolUse("2", R"({"action":"close", "tab_ids":[1]})")));
  auto content_only = ToolUse("3", R"({"action":"list"})");
  challenge = GetChallenge(tool, *content_only);
  ASSERT_TRUE(challenge);
  EXPECT_FALSE(challenge->plan);
  EXPECT_TRUE(challenge->implications);

  tool.UserPermissionGranted(content_only->id, *challenge);
  output = base::test::ParseJsonDict(
      RunToolAndGetText(FROM_HERE, &tool, R"({"action":"list"})"));
  EXPECT_TRUE(FindPassages(output, url));
}

// A grant for a challenge that never disclosed page content doesn't extend to
// it.
IN_PROC_BROWSER_TEST_F(TabManagementToolPageContentBrowserTest,
                       UnrelatedGrantDoesNotSendPassages) {
  TabManagementTool tool(profile());
  bool fetched = false;
  SetPassage(tool, kPassage,
             base::BindLambdaForTesting([&] { fetched = true; }));
  const GURL url("https://a.test/");
  AddTabAndGetHandle(browser(), url);

  tool.UserPermissionGranted(
      "server-challenge", *mojom::PermissionChallenge::New(
                              "off-topic", std::nullopt, std::nullopt,
                              std::nullopt, /*supports_allow_session=*/false));

  base::DictValue output = base::test::ParseJsonDict(
      RunToolAndGetText(FROM_HERE, &tool, kListInput));
  EXPECT_FALSE(FindPassages(output, url));
  EXPECT_FALSE(fetched);
  auto challenge = GetChallenge(tool, *ToolUse("1", kListInput));
  ASSERT_TRUE(challenge);
  EXPECT_TRUE(challenge->implications);
}

// The server's alignment check shows this tool's implications, so answering
// it grants page content and the list asks nothing more.
IN_PROC_BROWSER_TEST_F(TabManagementToolPageContentBrowserTest,
                       ServerChallengeWithImplicationsSendsPassages) {
  TabManagementTool tool(profile());
  SetPassage(tool, kPassage);
  const GURL url("https://a.test/");
  AddTabAndGetHandle(browser(), url);

  auto use = ToolUse("1", kListInput);
  auto implications = tool.GetPermissionChallengeImplications(*use);
  ASSERT_TRUE(implications);
  EXPECT_EQ(implications, GetChallenge(tool, *use)->implications);

  tool.UserPermissionGranted(
      use->id, *mojom::PermissionChallenge::New(
                   "off-topic", std::nullopt, std::nullopt, implications,
                   /*supports_allow_session=*/false));
  EXPECT_FALSE(GetChallenge(tool, *ToolUse("2", kListInput)));

  base::DictValue output = base::test::ParseJsonDict(
      RunToolAndGetText(FROM_HERE, &tool, kListInput));
  EXPECT_TRUE(FindPassages(output, url));

  SetSemanticHistorySearchEnabled(false);
  EXPECT_FALSE(tool.GetPermissionChallengeImplications(*use));
}

// A list is sent whole rather than in chunks, so it carries no more page
// excerpts than one Tab Focus chunk; tabs past that get none, even ones whose
// excerpts would still fit, so the tabs with excerpts lead the list.
IN_PROC_BROWSER_TEST_F(TabManagementToolPageContentBrowserTest,
                       PassagesStopAtTabListBudget) {
  TabManagementTool tool(profile());
  const GURL first("https://a.test/");
  const GURL second("https://b.test/");
  const GURL third("https://c.test/");
  // The first two tabs' excerpts each take just over half the budget, so only
  // the first fits. The third's are short enough to fit in what's left.
  tool.SetTabPassagesFetcherForTesting(base::BindLambdaForTesting(
      [third](const std::vector<GURL>& urls,
              base::OnceCallback<void(std::vector<std::vector<std::string>>)>
                  callback) {
        std::vector<std::vector<std::string>> passages;
        for (const GURL& url : urls) {
          passages.push_back({std::string(
              url == third ? 1 : kMaxPassageBytesPerTabList / 2 + 1, 'x')});
        }
        std::move(callback).Run(std::move(passages));
      }));
  AddTabAndGetHandle(browser(), first);
  AddTabAndGetHandle(browser(), second);
  AddTabAndGetHandle(browser(), third);

  auto use = ToolUse("1", kListInput);
  auto challenge = GetChallenge(tool, *use);
  ASSERT_TRUE(challenge);
  tool.UserPermissionGranted(use->id, *challenge);

  base::DictValue output = base::test::ParseJsonDict(
      RunToolAndGetText(FROM_HERE, &tool, kListInput));
  EXPECT_TRUE(FindPassages(output, first));
  EXPECT_FALSE(FindPassages(output, second));
  EXPECT_FALSE(FindPassages(output, third));
}

// A challenge grants only what it showed. When a later challenge reuses the ID
// of an unanswered one that disclosed page content, answering it grants tab
// access alone.
IN_PROC_BROWSER_TEST_F(TabManagementToolPageContentBrowserTest,
                       ReusedIdGrantsOnlyWhatItsChallengeShowed) {
  TabManagementTool tool(profile());
  bool fetched = false;
  SetPassage(tool, kPassage,
             base::BindLambdaForTesting([&] { fetched = true; }));
  const GURL url("https://a.test/");
  AddTabAndGetHandle(browser(), url);

  auto with_page_content = GetChallenge(tool, *ToolUse("1", kListInput));
  ASSERT_TRUE(with_page_content);
  ASSERT_TRUE(with_page_content->implications);

  SetSemanticHistorySearchEnabled(false);
  auto tabs_only = GetChallenge(tool, *ToolUse("1", kListInput));
  ASSERT_TRUE(tabs_only);
  EXPECT_FALSE(tabs_only->implications);
  tool.UserPermissionGranted("1", *tabs_only);

  // Page content is readable again, and still has to be asked for.
  SetSemanticHistorySearchEnabled(true);
  EXPECT_TRUE(GetChallenge(tool, *ToolUse("2", kListInput)));
  base::DictValue output = base::test::ParseJsonDict(
      RunToolAndGetText(FROM_HERE, &tool, kListInput));
  EXPECT_FALSE(FindPassages(output, url));
  EXPECT_FALSE(fetched);
}

// A page-content challenge left unanswered or denied can't be answered under
// its ID by a challenge that didn't disclose page content.
IN_PROC_BROWSER_TEST_F(TabManagementToolPageContentBrowserTest,
                       ServerChallengeReusingIdGrantsNoPageContent) {
  TabManagementTool tool(profile());
  bool fetched = false;
  SetPassage(tool, kPassage,
             base::BindLambdaForTesting([&] { fetched = true; }));
  const GURL url("https://a.test/");
  AddTabAndGetHandle(browser(), url);

  auto with_page_content = GetChallenge(tool, *ToolUse("1", kListInput));
  ASSERT_TRUE(with_page_content);
  ASSERT_TRUE(with_page_content->implications);

  tool.UserPermissionGranted(
      "1", *mojom::PermissionChallenge::New("off-topic", std::nullopt,
                                            std::nullopt, std::nullopt,
                                            /*supports_allow_session=*/false));

  EXPECT_TRUE(GetChallenge(tool, *ToolUse("2", kListInput)));
  base::DictValue output = base::test::ParseJsonDict(
      RunToolAndGetText(FROM_HERE, &tool, kListInput));
  EXPECT_FALSE(FindPassages(output, url));
  EXPECT_FALSE(fetched);
}

// Turning the index off withdraws page content even from a conversation that
// already allowed it: the next list reads nothing and asks nothing.
IN_PROC_BROWSER_TEST_F(TabManagementToolPageContentBrowserTest,
                       IndexTurnedOffAfterGrantStopsPassages) {
  TabManagementTool tool(profile());
  bool fetched = false;
  SetPassage(tool, kPassage,
             base::BindLambdaForTesting([&] { fetched = true; }));
  const GURL url("https://a.test/");
  AddTabAndGetHandle(browser(), url);

  auto use = ToolUse("1", kListInput);
  auto challenge = GetChallenge(tool, *use);
  ASSERT_TRUE(challenge);
  tool.UserPermissionGranted(use->id, *challenge);

  SetSemanticHistorySearchEnabled(false);

  EXPECT_FALSE(GetChallenge(tool, *ToolUse("2", kListInput)));
  base::DictValue output = base::test::ParseJsonDict(
      RunToolAndGetText(FROM_HERE, &tool, kListInput));
  EXPECT_FALSE(FindPassages(output, url));
  EXPECT_FALSE(fetched);
}

// The read is asynchronous, so the index going away while it is in flight
// drops what it returned.
IN_PROC_BROWSER_TEST_F(TabManagementToolPageContentBrowserTest,
                       IndexTurnedOffDuringReadDropsPassages) {
  TabManagementTool tool(profile());
  SetPassage(tool, kPassage, base::BindLambdaForTesting([&] {
               SetSemanticHistorySearchEnabled(false);
             }));
  const GURL url("https://a.test/");
  AddTabAndGetHandle(browser(), url);

  auto use = ToolUse("1", kListInput);
  auto challenge = GetChallenge(tool, *use);
  ASSERT_TRUE(challenge);
  tool.UserPermissionGranted(use->id, *challenge);

  base::DictValue output = base::test::ParseJsonDict(
      RunToolAndGetText(FROM_HERE, &tool, kListInput));
  EXPECT_FALSE(FindPassages(output, url));
}

#endif  // BUILDFLAG(ENABLE_LOCAL_AI)

}  // namespace ai_chat
