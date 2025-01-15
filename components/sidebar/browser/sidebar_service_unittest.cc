/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sidebar/browser/sidebar_service.h"

#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "base/containers/contains.h"
#include "base/json/json_reader.h"
#include "base/ranges/algorithm.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/values_test_util.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/sidebar/browser/constants.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "brave/components/sidebar/browser/sidebar_p3a.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/pref_service_mock_factory.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/version_info/channel.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/components/playlist/common/features.h"
#endif  // BUILDFLAG(ENABLE_PLAYLIST)

using ::testing::Eq;
using ::testing::NiceMock;
using ::testing::Optional;
using version_info::Channel;

namespace {

constexpr sidebar::SidebarItem::BuiltInItemType
    kDefaultBuiltInItemTypesForTest[] = {
        sidebar::SidebarItem::BuiltInItemType::kBraveTalk,
        sidebar::SidebarItem::BuiltInItemType::kWallet,
        sidebar::SidebarItem::BuiltInItemType::kChatUI,
        sidebar::SidebarItem::BuiltInItemType::kBookmarks,
        sidebar::SidebarItem::BuiltInItemType::kReadingList,
        sidebar::SidebarItem::BuiltInItemType::kHistory,
        sidebar::SidebarItem::BuiltInItemType::kPlaylist};

constexpr char sidebar_all_builtin_visible_json[] = R"({
        "hidden_built_in_items": [  ],
        "item_added_feedback_bubble_shown_count": 3,
        "side_panel_width": 320,
        "sidebar_alignment_changed_for_vertical_tabs": false,
        "sidebar_items": [ {
          "built_in_item_type": 7,
          "type": 0
        }, {
          "built_in_item_type": 2,
          "type": 0
        }, {
          "built_in_item_type": 4,
          "type": 0
        }, {
          "built_in_item_type": 3,
          "type": 0
        }, {
          "built_in_item_type": 1,
          "type": 0
        }, {
          "built_in_item_type": 0,
          "open_in_panel": false,
          "title": "Artificial intelligence - Wikipedia",
          "type": 1,
          "url": "https://en.wikipedia.org/wiki/Artificial_intelligence"
        }, {
          "built_in_item_type": 0,
          "open_in_panel": false,
          "title": "chrome.org",
          "type": 1,
          "url": "https://chrome.org/"
        }, {
          "built_in_item_type": 0,
          "open_in_panel": false,
          "title": "Google Chrome",
          "type": 1,
          "url": "https://www.google.com/chrome/"
        } ],
        "sidebar_show_option": 0
    })";
constexpr char sidebar_builtin_wallet_hidden_json[] = R"({
         "hidden_built_in_items": [ 2 ],
         "item_added_feedback_bubble_shown_count": 3,
         "side_panel_width": 320,
         "sidebar_alignment_changed_for_vertical_tabs": false,
         "sidebar_items": [ {
            "built_in_item_type": 1,
            "type": 0
         }, {
            "built_in_item_type": 3,
            "type": 0
         }, {
            "built_in_item_type": 4,
            "type": 0
         }, {
            "built_in_item_type": 7,
            "type": 0
         }, {
            "built_in_item_type": 0,
            "open_in_panel": false,
            "title": "chrome.org",
            "type": 1,
            "url": "https://chrome.org/"
         }, {
            "built_in_item_type": 0,
            "open_in_panel": false,
            "title": "Artificial intelligence - Wikipedia",
            "type": 1,
            "url": "https://en.wikipedia.org/wiki/Artificial_intelligence"
         }, {
            "built_in_item_type": 0,
            "open_in_panel": false,
            "title": "Google Chrome",
            "type": 1,
            "url": "https://www.google.com/chrome/"
         } ],
         "sidebar_show_option": 0
      })";
constexpr char sidebar_builtin_ai_chat_not_listed_json[] = R"({
         "hidden_built_in_items": [ ],
         "item_added_feedback_bubble_shown_count": 3,
         "side_panel_width": 320,
         "sidebar_alignment_changed_for_vertical_tabs": false,
         "sidebar_items": [ {
            "built_in_item_type": 1,
            "type": 0
         }, {
            "built_in_item_type": 3,
            "type": 0
         }, {
            "built_in_item_type": 4,
            "type": 0
         }, {
            "built_in_item_type": 2,
            "type": 0
         }, {
            "built_in_item_type": 0,
            "open_in_panel": false,
            "title": "chrome.org",
            "type": 1,
            "url": "https://chrome.org/"
         }, {
            "built_in_item_type": 0,
            "open_in_panel": false,
            "title": "Artificial intelligence - Wikipedia",
            "type": 1,
            "url": "https://en.wikipedia.org/wiki/Artificial_intelligence"
         }, {
            "built_in_item_type": 0,
            "open_in_panel": false,
            "title": "Google Chrome",
            "type": 1,
            "url": "https://www.google.com/chrome/"
         } ],
         "sidebar_show_option": 0
      })";
}  // namespace

namespace sidebar {

class MockSidebarServiceObserver : public SidebarService::Observer {
 public:
  MockSidebarServiceObserver() = default;
  ~MockSidebarServiceObserver() override = default;

  MOCK_METHOD(void,
              OnItemAdded,
              (const SidebarItem& item, size_t index),
              (override));
  MOCK_METHOD(void,
              OnWillRemoveItem,
              (const SidebarItem& item, size_t index),
              (override));
  MOCK_METHOD(void,
              OnItemRemoved,
              (const SidebarItem& item, size_t index),
              (override));
  MOCK_METHOD(void,
              OnItemUpdated,
              (const SidebarItem& item, const SidebarItemUpdate& update),
              (override));
  MOCK_METHOD(void,
              OnItemMoved,
              (const SidebarItem& item, size_t from, size_t to),
              (override));
};

class SidebarServiceTest : public testing::Test {
 public:
  SidebarServiceTest() = default;

  ~SidebarServiceTest() override = default;

  void SetUp() override {
    SidebarService::RegisterProfilePrefs(
        prefs_.registry(), SidebarService::ShowSidebarOption::kShowAlways);
  }
  void TearDown() override { ResetService(); }

  void InitService() {
    std::vector<SidebarItem::BuiltInItemType> default_item_types(
        std::begin(kDefaultBuiltInItemTypesForTest),
        std::end(kDefaultBuiltInItemTypesForTest));
    service_ = std::make_unique<SidebarService>(&prefs_, default_item_types);
    service_->AddObserver(&observer_);
  }

  void ResetService() {
    service_->RemoveObserver(&observer_);
    service_.reset();
  }

  PrefService* GetPrefs() { return &prefs_; }

  size_t GetDefaultItemCount() const {
    auto item_count =
        std::size(kDefaultBuiltInItemTypesForTest) - 1 /* for history*/;
#if BUILDFLAG(ENABLE_PLAYLIST)
    if (!base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
      item_count -= 1;
    }
#endif

    if (!ai_chat::features::IsAIChatEnabled()) {
      item_count -= 1;
    }

    return item_count;
  }

  TestingPrefServiceSimple prefs_;
  NiceMock<MockSidebarServiceObserver> observer_;
  std::unique_ptr<SidebarService> service_;
  base::HistogramTester histogram_tester_;
};

TEST_F(SidebarServiceTest, AddRemoveItems) {
  InitService();

  // Check the default items count.
  const auto default_item_count = GetDefaultItemCount();
  EXPECT_EQ(default_item_count, service_->items().size());
  EXPECT_EQ(0UL, service_->GetHiddenDefaultSidebarItems().size());

  // Cache 1st item to compare after removing.
  const SidebarItem item = service_->items()[0];
  EXPECT_TRUE(IsBuiltInType(item));

  EXPECT_CALL(observer_, OnWillRemoveItem(item, 0)).Times(1);
  EXPECT_CALL(observer_, OnItemRemoved(item, 0)).Times(1);
  service_->RemoveItemAt(0);
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_EQ(default_item_count - 1, service_->items().size());
  EXPECT_EQ(1UL, service_->GetHiddenDefaultSidebarItems().size());

  // Add again.
  // New item will be added as first,
  // according to the defined order
  EXPECT_CALL(observer_, OnItemAdded(item, 0)).Times(1);
  service_->AddItem(item);
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_EQ(default_item_count, service_->items().size());
  EXPECT_EQ(0UL, service_->GetHiddenDefaultSidebarItems().size());

  const SidebarItem item2 = SidebarItem::Create(
      GURL("https://www.brave.com/"), u"brave software",
      SidebarItem::Type::kTypeWeb, SidebarItem::BuiltInItemType::kNone, false);
  EXPECT_TRUE(IsWebType(item2));
  EXPECT_CALL(observer_, OnItemAdded(item2, default_item_count)).Times(1);
  service_->AddItem(item2);
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_EQ(default_item_count + 1, service_->items().size());
  // Default item count is not changed.
  EXPECT_EQ(default_item_count,
            service_->GetCurrentlyPresentBuiltInTypes().size());
}

TEST_F(SidebarServiceTest, MoveItem) {
  InitService();

  // Add one more item to test with 5 items.
  SidebarItem new_item = SidebarItem::Create(
      GURL("https://www.brave.com/"), u"brave software",
      SidebarItem::Type::kTypeWeb, SidebarItem::BuiltInItemType::kNone, false);
  service_->AddItem(new_item);
  EXPECT_EQ(GetDefaultItemCount() + 1, service_->items().size());

  // Move item at 0 to index 2.
  SidebarItem item = service_->items()[0];
  EXPECT_CALL(observer_, OnItemMoved(item, 0, 2)).Times(1);
  service_->MoveItem(0, 2);
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_EQ(item.url, service_->items()[2].url);

  // Move item at 0 to index 3.
  item = service_->items()[0];
  service_->MoveItem(0, 3);
  EXPECT_EQ(item.url, service_->items()[3].url);

  // Move item at 3 to index 0.
  item = service_->items()[3];
  service_->MoveItem(3, 0);
  EXPECT_EQ(item.url, service_->items()[0].url);

  // Move item at 2 to index 1.
  item = service_->items()[2];
  service_->MoveItem(2, 1);
  EXPECT_EQ(item.url, service_->items()[1].url);
}

TEST(SidebarItemTest, SidebarItemValidation) {
  SidebarItem builtin_item;
  EXPECT_FALSE(IsValidItem(builtin_item));

  builtin_item.type = SidebarItem::Type::kTypeBuiltIn;
  builtin_item.title = u"title";
  builtin_item.built_in_item_type = SidebarItem::BuiltInItemType::kNone;

  // builtin item should have have specific builtin item type.
  EXPECT_FALSE(IsValidItem(builtin_item));
  builtin_item.built_in_item_type = SidebarItem::BuiltInItemType::kBookmarks;
  EXPECT_TRUE(IsValidItem(builtin_item));

  // Invalid if title is empty.
  builtin_item.title = u"";
  EXPECT_FALSE(IsValidItem(builtin_item));
  builtin_item.title = u"title";
  EXPECT_TRUE(IsValidItem(builtin_item));

  SidebarItem web_item;
  web_item.type = SidebarItem::Type::kTypeWeb;
  web_item.built_in_item_type = SidebarItem::BuiltInItemType::kNone;
  web_item.url = GURL("https://abcd.com/");
  web_item.title = u"title";
  EXPECT_TRUE(IsValidItem(web_item));
  web_item.built_in_item_type = SidebarItem::BuiltInItemType::kBookmarks;
  EXPECT_FALSE(IsValidItem(web_item));
}

TEST_F(SidebarServiceTest, UpdateItem) {
  InitService();

  // Added webtype item at last.
  int last_item_index = service_->items().size() - 1;
  // Builtin type is not editable.
  EXPECT_TRUE(IsBuiltInType(service_->items()[last_item_index]));
  EXPECT_FALSE(service_->IsEditableItemAt(last_item_index));

  SidebarItem brave_item;
  const GURL brave_url("https://brave.com/");
  const std::u16string brave_title(u"Brave software");
  brave_item.url = brave_url;
  brave_item.title = brave_title;
  brave_item.type = SidebarItem::Type::kTypeWeb;
  brave_item.built_in_item_type = SidebarItem::BuiltInItemType::kNone;
  service_->AddItem(brave_item);
  last_item_index++;
  EXPECT_EQ(brave_url, service_->items()[last_item_index].url);
  EXPECT_TRUE(service_->IsEditableItemAt(last_item_index));

  SidebarItemUpdate update;
  update.index = last_item_index;
  // Update title only.
  update.title_updated = true;
  update.url_updated = false;
  EXPECT_CALL(observer_, OnItemUpdated(testing::_, update)).Times(1);
  service_->UpdateItem(brave_url, brave_url, brave_title, u"Brave");
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_EQ(u"Brave", service_->items()[last_item_index].title);

  // Update title & url.
  update.title_updated = true;
  update.url_updated = true;
  EXPECT_CALL(observer_, OnItemUpdated(testing::_, update)).Times(1);
  service_->UpdateItem(brave_url, GURL("https://a.com/"), u"Brave", u"a");
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_EQ(u"a", service_->items()[last_item_index].title);
  EXPECT_EQ(GURL("https://a.com/"), service_->items()[last_item_index].url);

  // Added one more webtype item.
  service_->AddItem(brave_item);
  last_item_index++;

  // Trying to update the url that another item already uses and
  // check item is not updated and it still has brave url.
  EXPECT_CALL(observer_, OnItemUpdated(testing::_, testing::_)).Times(0);
  service_->UpdateItem(brave_url, GURL("https://a.com/"), u"a", u"ab");
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_EQ(brave_url, service_->items()[last_item_index].url);
}

TEST_F(SidebarServiceTest, MoveItemSavedToPrefs) {
  InitService();

  // Add one more item to test.
  const auto expected_item_count = GetDefaultItemCount() + 1;
  SidebarItem new_item = SidebarItem::Create(
      GURL("https://www.brave.com/"), u"brave software",
      SidebarItem::Type::kTypeWeb, SidebarItem::BuiltInItemType::kNone, false);
  service_->AddItem(new_item);
  EXPECT_EQ(expected_item_count, service_->items().size());

  // Move item at 0 to index 2.
  SidebarItem item = service_->items()[0];
  GURL url = item.url;
  EXPECT_CALL(observer_, OnItemMoved(item, 0, 2)).Times(1);
  service_->MoveItem(0, 2);
  testing::Mock::VerifyAndClearExpectations(&observer_);

  ResetService();
  InitService();
  EXPECT_EQ(expected_item_count, service_->items().size());
  EXPECT_EQ(url, service_->items()[2].url);
}

TEST_F(SidebarServiceTest, HideBuiltInItem) {
  // Have prefs which contain a custom item and hides 1 built-in item
  {
    base::Value::List list;
    list.Append(static_cast<int>(SidebarItem::BuiltInItemType::kBookmarks));
    prefs_.SetList(sidebar::kSidebarHiddenBuiltInItems, std::move(list));
  }
  {
    base::Value::Dict dict;
    dict.Set(sidebar::kSidebarItemURLKey, "https://custom1.brave.com/");
    dict.Set(sidebar::kSidebarItemTitleKey, "Custom Item 1");
    dict.Set(sidebar::kSidebarItemTypeKey,
             static_cast<int>(SidebarItem::Type::kTypeWeb));
    dict.Set(sidebar::kSidebarItemOpenInPanelKey, false);

    base::Value::List list;
    list.Append(std::move(dict));
    prefs_.SetList(sidebar::kSidebarItems, std::move(list));
  }

  InitService();
  // None of the items should be the hidden one
  auto items = service_->items();
  EXPECT_FALSE(base::Contains(items, SidebarItem::BuiltInItemType::kBookmarks,
                              &SidebarItem::built_in_item_type));
  // Check serialization also perists that
  ResetService();
  InitService();
  items = service_->items();
  EXPECT_FALSE(base::Contains(items, SidebarItem::BuiltInItemType::kBookmarks,
                              &SidebarItem::built_in_item_type));
}

TEST_F(SidebarServiceTest, NewDefaultItemAdded) {
  std::vector<SidebarItem::BuiltInItemType> hidden_builtin_types{
      SidebarItem::BuiltInItemType::kBookmarks};
  // Have prefs which contain a custom item and hides 1 built-in item
  {
    base::Value::List list;
    base::ranges::for_each(hidden_builtin_types, [&list](const auto& item) {
      list.Append(static_cast<int>(item));
    });
    prefs_.SetList(sidebar::kSidebarHiddenBuiltInItems, std::move(list));
  }
  {
    base::Value::Dict dict;
    dict.Set(sidebar::kSidebarItemURLKey, "https://custom1.brave.com/");
    dict.Set(sidebar::kSidebarItemTitleKey, "Custom Item 1");
    dict.Set(sidebar::kSidebarItemTypeKey,
             static_cast<int>(SidebarItem::Type::kTypeWeb));
    dict.Set(sidebar::kSidebarItemOpenInPanelKey, false);

    base::Value::List list;
    list.Append(std::move(dict));
    prefs_.SetList(sidebar::kSidebarItems, std::move(list));
  }

  InitService();
  // Make sure other default items are present. They are considered not seen yet
  // since kSidebarItems was not default value and did not contain them.
  // None of the items should be the hidden one
  auto items = service_->items();
  EXPECT_FALSE(base::Contains(items, SidebarItem::BuiltInItemType::kBookmarks,
                              &SidebarItem::built_in_item_type));
  // All other default items should be present even though not present
  // in kSidebarItems pref.
  std::vector<SidebarItem::BuiltInItemType> default_items;
  base::ranges::copy_if(
      kDefaultBuiltInItemTypesForTest, std::back_inserter(default_items),
      [&hidden_builtin_types](const auto& built_in_type) {
        if (base::Contains(hidden_builtin_types, built_in_type)) {
          // Hidden by preference
          return false;
        }

        if (built_in_type == SidebarItem::BuiltInItemType::kHistory) {
          // Currently, we don't show kHistory item regardless of the
          // preference.
          return false;
        }

        if (!ai_chat::features::IsAIChatEnabled() &&
            built_in_type == SidebarItem::BuiltInItemType::kChatUI) {
          return false;
        }

#if BUILDFLAG(ENABLE_PLAYLIST)
        if (!base::FeatureList::IsEnabled(playlist::features::kPlaylist) &&
            built_in_type == SidebarItem::BuiltInItemType::kPlaylist) {
          return false;
        }
#endif
        return true;
      });

  // There should also be the custom item we added
  EXPECT_EQ(default_items.size() + 1, service_->items().size());

  // Get expected indexes (excluding the hidden items).
  const auto custom_item_index = std::distance(
      items.begin(),
      base::ranges::find(items, SidebarItem::BuiltInItemType::kNone,
                         &SidebarItem::built_in_item_type));

  for (auto built_in_type : default_items) {
    auto expected_index =
        std::distance(std::begin(default_items),
                      base::ranges::find(default_items, built_in_type));
    if (expected_index >= custom_item_index) {
      expected_index++;
    }

    auto iter = base::ranges::find(items, built_in_type,
                                   &SidebarItem::built_in_item_type);
    EXPECT_NE(iter, items.end());
    auto actual_index = std::distance(items.begin(), iter);

    EXPECT_EQ(expected_index, actual_index)
        << "New item with ID " << static_cast<int>(built_in_type)
        << " was inserted at unexpected index";
  }
}

TEST_F(SidebarServiceTest, MigratePrefSidebarBuiltInItemsSomeHidden) {
  // Make prefs already have old-style builtin items before service
  // initialization.
  {
    base::Value::Dict dict;
    dict.Set(sidebar::kSidebarItemURLKey, "https://anything.brave.com/");
    dict.Set(sidebar::kSidebarItemTitleKey, "Anything");
    dict.Set(sidebar::kSidebarItemTypeKey,
             static_cast<int>(SidebarItem::Type::kTypeBuiltIn));
    dict.Set(sidebar::kSidebarItemBuiltInItemTypeKey,
             static_cast<int>(SidebarItem::BuiltInItemType::kBraveTalk));
    dict.Set(sidebar::kSidebarItemOpenInPanelKey, true);

    base::Value::List list;
    list.Append(std::move(dict));
    prefs_.SetList(sidebar::kSidebarItems, std::move(list));
  }

  InitService();

  // Verify service has migrated the "include" of kBraveTalk to be the exclude
  // of all other built-in items that were known prior to this migration and are
  // still available.
  auto* preference = prefs_.FindPreference(kSidebarItems);
  auto* hidden_preference = prefs_.FindPreference(kSidebarHiddenBuiltInItems);
  EXPECT_EQ(1UL, preference->GetValue()->GetList().size())
      << "Still contains built-in item in original preference, for ordering "
         "purposes";
  EXPECT_EQ(2UL, hidden_preference->GetValue()->GetList().size())
      << "Migration resulted in hiding the expected number of items";
  // Verify expected items
  EXPECT_EQ(GetDefaultItemCount() - 2UL, service_->items().size());
  auto items = service_->items();
  auto talk_iter =
      base::ranges::find(items, SidebarItem::BuiltInItemType::kBraveTalk,
                         &SidebarItem::built_in_item_type);
  EXPECT_NE(talk_iter, items.end());
  EXPECT_TRUE(base::Contains(items, SidebarItem::BuiltInItemType::kReadingList,
                             &SidebarItem::built_in_item_type));
  // Check service has updated built-in item. Previously url was incorrect. This
  // check is to make sure that we don't re-introduce code which stores the URL
  // for built-in items.
  auto talk_item = *talk_iter;
  EXPECT_EQ(talk_item.url, kBraveTalkURL);
}

TEST_F(SidebarServiceTest, MigratePrefSidebarBuiltInItemsNoneHidden) {
  // Make prefs already have ALL old-style builtin items before service
  // initialization. This tests that when not adding anything to the new pref
  // that re-migration will not break anything.
  // Also add a custom item so that the main items pref is not default value.
  {
    std::vector<SidebarItem::BuiltInItemType> hideable_types{
        SidebarItem::BuiltInItemType::kBraveTalk,
        SidebarItem::BuiltInItemType::kWallet,
        SidebarItem::BuiltInItemType::kBookmarks,
    };

    base::Value::List list;
    for (const auto& built_in_type : hideable_types) {
      base::Value::Dict dict;
      dict.Set(sidebar::kSidebarItemURLKey, "https://anything.brave.com/");
      dict.Set(sidebar::kSidebarItemTitleKey, "Anything");
      dict.Set(sidebar::kSidebarItemTypeKey,
               static_cast<int>(SidebarItem::Type::kTypeBuiltIn));
      dict.Set(sidebar::kSidebarItemBuiltInItemTypeKey,
               static_cast<int>(built_in_type));
      dict.Set(sidebar::kSidebarItemOpenInPanelKey, true);
      list.Append(std::move(dict));
    }

    base::Value::Dict dict;
    dict.Set(sidebar::kSidebarItemURLKey, "https://custom1.brave.com/");
    dict.Set(sidebar::kSidebarItemTitleKey, "Custom Item 1");
    dict.Set(sidebar::kSidebarItemTypeKey,
             static_cast<int>(SidebarItem::Type::kTypeWeb));
    dict.Set(sidebar::kSidebarItemOpenInPanelKey, false);

    list.Append(std::move(dict));
    prefs_.SetList(sidebar::kSidebarItems, std::move(list));
  }

  InitService();

  // Verify service has migrated the "include" of kBraveTalk to be the exclude
  // of all other built-in items that were known prior to this migration and are
  // still available.
  auto* preference = prefs_.FindPreference(kSidebarItems);
  auto* hidden_preference = prefs_.FindPreference(kSidebarHiddenBuiltInItems);
  EXPECT_EQ(4UL, preference->GetValue()->GetList().size())
      << "Migration still contains built-in items, for ordering purposes";
  EXPECT_EQ(0UL, hidden_preference->GetValue()->GetList().size())
      << "Migration did not result in hiding any built-in items";
  // kSidebarHiddenBuiltInItems being non-default is how we know migration is
  // already done.
  EXPECT_FALSE(hidden_preference->IsDefaultValue());
  // kSidebarItems should still have custom item
  EXPECT_FALSE(preference->IsDefaultValue());
  // Verify expected items = items in pref plus new item (reading list)
  const auto default_item_count = GetDefaultItemCount();
  EXPECT_EQ(default_item_count + 1, service_->items().size());
  // Simulate re-launch and check service has still updated items.
  // This tests re-migration doesn't occur and hide all the hideable built-in
  // items.
  ResetService();
  InitService();
  // Prefs still haven't been re-serialized, so don't contain new items
  EXPECT_EQ(4UL, preference->GetValue()->GetList().size());
  EXPECT_EQ(0UL, hidden_preference->GetValue()->GetList().size());
  EXPECT_FALSE(hidden_preference->IsDefaultValue());
  EXPECT_FALSE(preference->IsDefaultValue());
  EXPECT_EQ(default_item_count + 1, service_->items().size());

  // Check again after service updates prefs. Force serialization by performing
  // a move operation (and move it back).
  service_->MoveItem(0, 1);
  service_->MoveItem(0, 1);
  ResetService();
  InitService();
  // Pref now includes new default items added after migration (ReadingList),
  // so size has increased by 1.
  EXPECT_EQ(default_item_count + 1, preference->GetValue()->GetList().size());
  EXPECT_EQ(0UL, hidden_preference->GetValue()->GetList().size());
  EXPECT_FALSE(hidden_preference->IsDefaultValue());
  EXPECT_FALSE(preference->IsDefaultValue());
  EXPECT_EQ(default_item_count + 1, service_->items().size());
  // Verify that new a new item not contained in prefs was added at correct
  // index.
  auto items = service_->items();
  auto iter =
      base::ranges::find(items, SidebarItem::BuiltInItemType::kReadingList,
                         &SidebarItem::built_in_item_type);
  auto index = iter - items.begin();
  EXPECT_EQ(4, index);
}

// Verify service has migrated the previous pref format where built-in items
// had url stored and not built-in-item-type.
TEST_F(SidebarServiceTest, MigratePrefSidebarBuiltInItemsNoType) {
  // Make prefs already have old-style builtin items before service
  // initialization.
  {
    // Items should not receive a built-in-item-type.
    std::vector<std::string> urls{
        "https://together.brave.com/",
        "chrome://wallet/",
        "chrome://bookmarks/",
        "chrome://history/",
    };
    base::Value::List list;
    for (const auto& url : urls) {
      base::Value::Dict dict;
      dict.Set(sidebar::kSidebarItemURLKey, url);
      dict.Set(sidebar::kSidebarItemTitleKey, "Anything");
      dict.Set(sidebar::kSidebarItemTypeKey,
               static_cast<int>(SidebarItem::Type::kTypeBuiltIn));
      dict.Set(sidebar::kSidebarItemOpenInPanelKey, true);
      list.Append(base::Value(std::move(dict)));
    }
    // Add a custom item to make sure we don't interfere with it
    base::Value::Dict dict;
    dict.Set(sidebar::kSidebarItemURLKey, "chrome://settings/help");
    dict.Set(sidebar::kSidebarItemTitleKey, "Anything");
    dict.Set(sidebar::kSidebarItemTypeKey,
             static_cast<int>(SidebarItem::Type::kTypeWeb));
    dict.Set(sidebar::kSidebarItemOpenInPanelKey, false);
    list.Append(std::move(dict));
    prefs_.SetList(sidebar::kSidebarItems, std::move(list));
  }

  // Not crashing is a good indicator this test has passed
  InitService();

  // Verify migration
  auto& items = prefs_.GetList(kSidebarItems);
  for (const auto& item : items) {
    const auto item_type = static_cast<SidebarItem::Type>(
        *item.GetDict().FindInt(kSidebarItemTypeKey));
    if (item_type == SidebarItem::Type::kTypeBuiltIn) {
      const auto built_in_type = static_cast<SidebarItem::BuiltInItemType>(
          *item.GetDict().FindInt(kSidebarItemBuiltInItemTypeKey));
      EXPECT_NE(built_in_type, SidebarItem::BuiltInItemType::kNone);
    }
  }

  // Verify the expected item count includes all default items (since all are
  // included in the pref, above), minus the obsolete items (history), plus any
  // new default items, plus the custom item.
  EXPECT_EQ(service_->items().size(),
            GetDefaultItemCount() + 1 /*for custom item added above*/);
}

TEST_F(SidebarServiceTest, HidesBuiltInItemsViaPref) {
  // Verify default state
  InitService();
  auto items = service_->items();
  EXPECT_TRUE(base::Contains(items, SidebarItem::BuiltInItemType::kBookmarks,
                             &SidebarItem::built_in_item_type));

  // Update pref to hide bookmarks item
  // Make prefs already have old-style builtin items before service
  // initialization.
  {
    base::Value::List list;
    list.Append(static_cast<int>(SidebarItem::BuiltInItemType::kBookmarks));
    prefs_.SetList(sidebar::kSidebarHiddenBuiltInItems, std::move(list));
  }

  // Verify new state doesn't include bookmarks item
  InitService();
  items = service_->items();
  EXPECT_FALSE(base::Contains(items, SidebarItem::BuiltInItemType::kBookmarks,
                              &SidebarItem::built_in_item_type));
}

TEST_F(SidebarServiceTest, HidesBuiltInItemsViaService) {
  // Verify default state
  InitService();
  auto items = service_->items();
  auto bookmark_item_index = 0u;
  bool found = false;
  while (bookmark_item_index < items.size()) {
    auto item = items[bookmark_item_index];
    if (item.built_in_item_type == SidebarItem::BuiltInItemType::kBookmarks) {
      found = true;
      break;
    }
    bookmark_item_index++;
  }
  EXPECT_TRUE(found);
  // Update service to hide bookmarks item
  service_->RemoveItemAt(bookmark_item_index);

  // Verify new state doesn't include bookmarks item
  ResetService();
  InitService();
  items = service_->items();
  EXPECT_FALSE(base::Contains(items, SidebarItem::BuiltInItemType::kBookmarks,
                              &SidebarItem::built_in_item_type));
}

TEST_F(SidebarServiceTest, BuiltInItemUpdateTestWithBuiltInItemTypeKey) {
  // Make prefs already have builtin items before service initialization.
  // And it has old url in old pref format (storing built-in items).
  {
    base::Value::Dict dict;
    dict.Set(sidebar::kSidebarItemURLKey, "https://deprecated.brave.com/");
    dict.Set(sidebar::kSidebarItemTitleKey, "Brave together");
    dict.Set(sidebar::kSidebarItemTypeKey,
             static_cast<int>(SidebarItem::Type::kTypeBuiltIn));
    dict.Set(sidebar::kSidebarItemBuiltInItemTypeKey,
             static_cast<int>(SidebarItem::BuiltInItemType::kBraveTalk));
    dict.Set(sidebar::kSidebarItemOpenInPanelKey, true);
    base::Value::List list;
    list.Append(std::move(dict));
    prefs_.SetList(sidebar::kSidebarItems, std::move(list));
  }

  InitService();

  // Brave Talk and Reading list.
  auto expected_count = 2UL;
#if BUILDFLAG(ENABLE_PLAYLIST)
  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    expected_count += 1;
  }
#endif

  if (ai_chat::features::IsAIChatEnabled()) {
    expected_count += 1;
  }

  // Check service has updated built-in item. Previously url was deprecated.xxx.
  EXPECT_EQ(expected_count, service_->items().size());
  EXPECT_EQ(GURL(kBraveTalkURL), service_->items()[0].url);

  // Simulate re-launch and check service has still updated items.
  ResetService();

  InitService();

  // Check service has updated built-in item. Previously url was deprecated.xxx.
  EXPECT_EQ(expected_count, service_->items().size());
  EXPECT_EQ(GURL(kBraveTalkURL), service_->items()[0].url);
}

TEST_F(SidebarServiceTest, BuiltInItemDoesntHaveHistoryItem) {
  // Make prefs already have builtin items before service initialization.
  // And it has history item.
  {
    base::Value::Dict dict;
    dict.Set(sidebar::kSidebarItemURLKey, "https://deprecated.brave.com/");
    dict.Set(sidebar::kSidebarItemTypeKey,
             static_cast<int>(SidebarItem::Type::kTypeBuiltIn));
    dict.Set(sidebar::kSidebarItemBuiltInItemTypeKey,
             static_cast<int>(SidebarItem::BuiltInItemType::kHistory));
    dict.Set(sidebar::kSidebarItemOpenInPanelKey, true);

    base::Value::List list;
    list.Append(std::move(dict));
    prefs_.SetList(sidebar::kSidebarItems, std::move(list));
  }

  InitService();

  // Check service doesn't have history item.
  for (const auto& item : service_->items()) {
    EXPECT_NE(SidebarItem::BuiltInItemType::kHistory, item.built_in_item_type);
  }
  // Make sure history is not included in the not added default items list.
  auto not_added_default_items = service_->GetHiddenDefaultSidebarItems();
  for (const auto& item : not_added_default_items) {
    EXPECT_NE(SidebarItem::BuiltInItemType::kHistory, item.built_in_item_type);
  }
}

TEST_F(SidebarServiceTest, SidebarShowOptionsDeprecationTest) {
  // Show on click is deprecated.
  // Treat it as a show on mouse over.
  prefs_.SetInteger(
      kSidebarShowOption,
      static_cast<int>(SidebarService::ShowSidebarOption::kShowOnClick));

  InitService();
  EXPECT_EQ(SidebarService::ShowSidebarOption::kShowOnMouseOver,
            service_->GetSidebarShowOption());
}

TEST_F(SidebarServiceTest, SidebarShowOptionsDefaultTestNonStable) {
  InitService();
  EXPECT_EQ(SidebarService::ShowSidebarOption::kShowAlways,
            service_->GetSidebarShowOption());
}

TEST_F(SidebarServiceTest, SidebarEnabledHistogram) {
  InitService();
  histogram_tester_.ExpectUniqueSample(p3a::kSidebarEnabledHistogramName, 1, 1);

  prefs_.SetInteger(
      kSidebarShowOption,
      static_cast<int>(SidebarService::ShowSidebarOption::kShowOnClick));
  histogram_tester_.ExpectUniqueSample(p3a::kSidebarEnabledHistogramName, 1, 2);

  prefs_.SetInteger(
      kSidebarShowOption,
      static_cast<int>(SidebarService::ShowSidebarOption::kShowNever));
  histogram_tester_.ExpectBucketCount(p3a::kSidebarEnabledHistogramName,
                                      INT_MAX - 1, 1);

  prefs_.SetInteger(
      kSidebarShowOption,
      static_cast<int>(SidebarService::ShowSidebarOption::kShowAlways));
  histogram_tester_.ExpectBucketCount(p3a::kSidebarEnabledHistogramName, 1, 3);
  histogram_tester_.ExpectTotalCount(p3a::kSidebarEnabledHistogramName, 4);
}

class SidebarServiceTestWithPlaylist : public SidebarServiceTest {
 public:
  SidebarServiceTestWithPlaylist() {
    scoped_feature_list_.InitAndEnableFeature(playlist::features::kPlaylist);
  }
  ~SidebarServiceTestWithPlaylist() override = default;

  bool SidebarHasDefaultPanelItem() const {
    const auto items = service_->items();
    auto iter = base::ranges::find_if(items, [](const SidebarItem& item) {
      return item.type == SidebarItem::Type::kTypeBuiltIn && item.open_in_panel;
    });
    return iter != items.end();
  }

  void RemoveAnySidebarPanelItem() {
    const auto items = service_->items();
    auto iter = base::ranges::find_if(items, [](const SidebarItem& item) {
      return item.type == SidebarItem::Type::kTypeBuiltIn && item.open_in_panel;
    });
    if (iter == items.end()) {
      return;
    }

    const int index = std::distance(items.begin(), iter);
    service_->RemoveItemAt(index);
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

// Check GetDefaultPanelItem() returns valid panel item.
TEST_F(SidebarServiceTestWithPlaylist, GetDefaultPanelItem) {
  InitService();

  while (SidebarHasDefaultPanelItem()) {
    EXPECT_TRUE(service_->GetDefaultPanelItem());
    RemoveAnySidebarPanelItem();
  }

  EXPECT_FALSE(service_->GetDefaultPanelItem());
}

class SidebarServiceOrderingTest : public SidebarServiceTest {
 public:
  SidebarServiceOrderingTest() = default;
  SidebarServiceOrderingTest(const SidebarServiceOrderingTest&) = delete;
  SidebarServiceOrderingTest& operator=(const SidebarServiceOrderingTest&) =
      delete;
  SidebarServiceOrderingTest(const SidebarServiceOrderingTest&&) = delete;
  SidebarServiceOrderingTest& operator=(const SidebarServiceOrderingTest&&) =
      delete;

  ~SidebarServiceOrderingTest() override = default;

  void SetUp() override {
    SidebarServiceTest::SetUp();
    scoped_feature_list_.InitAndEnableFeature(ai_chat::features::kAIChat);
  }

  bool ValidateBuiltInTypesOrdering(
      const std::vector<SidebarItem::BuiltInItemType>& defined_order) {
    size_t srv_items_index = 0, dbt_index = 0;
    const auto default_btin_types_count = defined_order.size();
    std::vector<SidebarItem> only_builtin_types;
    base::ranges::copy_if(
        service_->items(), std::back_inserter(only_builtin_types),
        [](const SidebarItem& item) {
          return item.built_in_item_type != SidebarItem::BuiltInItemType::kNone;
        });

    const auto srv_items_count = only_builtin_types.size();
    while (srv_items_index < srv_items_count &&
           dbt_index < default_btin_types_count) {
      if (only_builtin_types[srv_items_index].built_in_item_type ==
          defined_order[dbt_index]) {
        srv_items_index++;
      }
      dbt_index++;
    }
    return srv_items_index == srv_items_count;
  }

  void LoadFromPrefsTest(
      const base::Value::Dict& sidebar_prefs,
      const std::vector<SidebarItem::BuiltInItemType>& defined_order,
      const size_t expected_items_loaded) {
    GetPrefs()->Set(kSidebarItems,
                    std::move(sidebar_prefs.Find("sidebar_items")->Clone()));

    InitService();

    EXPECT_EQ(expected_items_loaded, service_->items().size());
    EXPECT_TRUE(ValidateBuiltInTypesOrdering(defined_order))
        << "Wrong order detected";
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(SidebarServiceOrderingTest, BuiltInItemsDefaultOrder) {
  InitService();
  EXPECT_EQ(GetDefaultItemCount(), service_->items().size());
  EXPECT_EQ(0UL, service_->GetHiddenDefaultSidebarItems().size());

  EXPECT_TRUE(
      ValidateBuiltInTypesOrdering({SidebarItem::BuiltInItemType::kBraveTalk,
                                    SidebarItem::BuiltInItemType::kWallet,
                                    SidebarItem::BuiltInItemType::kChatUI,
                                    SidebarItem::BuiltInItemType::kBookmarks,
                                    SidebarItem::BuiltInItemType::kReadingList,
                                    SidebarItem::BuiltInItemType::kHistory,
                                    SidebarItem::BuiltInItemType::kPlaylist}));
}

TEST_F(SidebarServiceOrderingTest, LoadFromPrefsAllBuiltInVisible) {
  base::Value::Dict sidebar =
      base::test::ParseJsonDict(sidebar_all_builtin_visible_json);

  const auto* sidebar_items = sidebar.FindList("sidebar_items");
  CHECK(sidebar_items);

  std::vector items = {
      SidebarItem::BuiltInItemType::kChatUI,
      SidebarItem::BuiltInItemType::kWallet,
      SidebarItem::BuiltInItemType::kReadingList,
      SidebarItem::BuiltInItemType::kBookmarks,
      SidebarItem::BuiltInItemType::kBraveTalk,
  };

  auto expected_count = sidebar_items->size();

  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    expected_count++;
    items.push_back(SidebarItem::BuiltInItemType::kPlaylist);
  }
  LoadFromPrefsTest(std::move(sidebar), items, expected_count);
}

TEST_F(SidebarServiceOrderingTest, LoadFromPrefsWalletBuiltInHidden) {
  base::Value::Dict sidebar =
      base::test::ParseJsonDict(sidebar_builtin_wallet_hidden_json);

  const auto* sidebar_items = sidebar.FindList("sidebar_items");
  CHECK(sidebar_items);

  std::vector items = {
      SidebarItem::BuiltInItemType::kBraveTalk,
      SidebarItem::BuiltInItemType::kBookmarks,
      SidebarItem::BuiltInItemType::kReadingList,
      SidebarItem::BuiltInItemType::kChatUI,
      SidebarItem::BuiltInItemType::kPlaylist,
  };

  auto expected_count = sidebar_items->size();

#if BUILDFLAG(ENABLE_PLAYLIST)
  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    expected_count++;
    items.push_back(SidebarItem::BuiltInItemType::kPlaylist);
  }
#endif

  LoadFromPrefsTest(std::move(sidebar), items, expected_count);
}

TEST_F(SidebarServiceOrderingTest, LoadFromPrefsAIChatBuiltInNotListed) {
  base::Value::Dict sidebar =
      base::test::ParseJsonDict(sidebar_builtin_ai_chat_not_listed_json);

  const auto* sidebar_items = sidebar.FindList("sidebar_items");
  CHECK(sidebar_items);

  std::vector items = {
      SidebarItem::BuiltInItemType::kBraveTalk,
      SidebarItem::BuiltInItemType::kBookmarks,
      SidebarItem::BuiltInItemType::kChatUI,
      SidebarItem::BuiltInItemType::kReadingList,
      SidebarItem::BuiltInItemType::kWallet,
  };

  auto expected_count = sidebar_items->size() + 1;

#if BUILDFLAG(ENABLE_PLAYLIST)
  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    expected_count++;
    items.push_back(SidebarItem::BuiltInItemType::kPlaylist);
  }
#endif

  LoadFromPrefsTest(std::move(sidebar), items, expected_count);
}

}  // namespace sidebar
