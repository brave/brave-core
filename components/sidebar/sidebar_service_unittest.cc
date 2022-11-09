/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/containers/contains.h"
#include "base/ranges/algorithm.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/playlist/features.h"
#include "brave/components/sidebar/constants.h"
#include "brave/components/sidebar/pref_names.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "components/version_info/channel.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::Eq;
using ::testing::NiceMock;
using ::testing::Optional;
using version_info::Channel;

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

  void TearDown() override { ResetService(); }

  void InitService() {
    service_ = std::make_unique<SidebarService>(&prefs_);
    service_->AddObserver(&observer_);
  }

  void ResetService() {
    service_->RemoveObserver(&observer_);
    service_.reset();
  }

  TestingPrefServiceSimple prefs_;
  NiceMock<MockSidebarServiceObserver> observer_;
  std::unique_ptr<SidebarService> service_;
};

TEST_F(SidebarServiceTest, AddRemoveItems) {
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::CANARY);
  InitService();

  // Check the default items count.
  EXPECT_EQ(4UL, service_->items().size());
  EXPECT_EQ(0UL, service_->GetHiddenDefaultSidebarItems().size());

  // Cache 1st item to compare after removing.
  const SidebarItem item = service_->items()[0];
  EXPECT_TRUE(IsBuiltInType(item));

  EXPECT_CALL(observer_, OnWillRemoveItem(item, 0)).Times(1);
  EXPECT_CALL(observer_, OnItemRemoved(item, 0)).Times(1);
  service_->RemoveItemAt(0);
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_EQ(3UL, service_->items().size());
  EXPECT_EQ(1UL, service_->GetHiddenDefaultSidebarItems().size());

  // Add again.
  // New item will be added at last.
  EXPECT_CALL(observer_, OnItemAdded(item, 3)).Times(1);
  service_->AddItem(item);
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_EQ(4UL, service_->items().size());
  EXPECT_EQ(0UL, service_->GetHiddenDefaultSidebarItems().size());

  const SidebarItem item2 = SidebarItem::Create(
      GURL("https://www.brave.com/"), u"brave software",
      SidebarItem::Type::kTypeWeb, SidebarItem::BuiltInItemType::kNone, false);
  EXPECT_TRUE(IsWebType(item2));
  EXPECT_CALL(observer_, OnItemAdded(item2, 4)).Times(1);
  service_->AddItem(item2);
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_EQ(5UL, service_->items().size());
  // Default item count is not changed.
  EXPECT_EQ(4UL, service_->GetCurrentlyPresentBuiltInTypes().size());
}

TEST_F(SidebarServiceTest, MoveItem) {
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::DEV);
  InitService();

  // Add one more item to test with 5 items.
  SidebarItem new_item = SidebarItem::Create(
      GURL("https://www.brave.com/"), u"brave software",
      SidebarItem::Type::kTypeWeb, SidebarItem::BuiltInItemType::kNone, false);
  service_->AddItem(new_item);
  EXPECT_EQ(5UL, service_->items().size());

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
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::DEV);
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
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::DEV);
  InitService();

  // Add one more item to test with 4 items.
  SidebarItem new_item = SidebarItem::Create(
      GURL("https://www.brave.com/"), u"brave software",
      SidebarItem::Type::kTypeWeb, SidebarItem::BuiltInItemType::kNone, false);
  service_->AddItem(new_item);
  EXPECT_EQ(5UL, service_->items().size());

  // Move item at 0 to index 2.
  SidebarItem item = service_->items()[0];
  GURL url = item.url;
  EXPECT_CALL(observer_, OnItemMoved(item, 0, 2)).Times(1);
  service_->MoveItem(0, 2);
  testing::Mock::VerifyAndClearExpectations(&observer_);

  ResetService();
  InitService();
  EXPECT_EQ(5UL, service_->items().size());
  EXPECT_EQ(url, service_->items()[2].url);
}

TEST_F(SidebarServiceTest, HideBuiltInItem) {
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::DEV);
  // Have prefs which contain a custom item and hides 1 built-in item
  {
    ListPrefUpdate update(&prefs_, sidebar::kSidebarHiddenBuiltInItems);
    update->ClearList();
    update->Append(static_cast<int>(SidebarItem::BuiltInItemType::kBookmarks));
  }
  {
    ListPrefUpdate update(&prefs_, sidebar::kSidebarItems);
    base::Value::Dict dict;
    dict.Set(sidebar::kSidebarItemURLKey, "https://custom1.brave.com/");
    dict.Set(sidebar::kSidebarItemTitleKey, "Custom Item 1");
    dict.Set(sidebar::kSidebarItemTypeKey,
             static_cast<int>(SidebarItem::Type::kTypeWeb));
    dict.Set(sidebar::kSidebarItemOpenInPanelKey, false);
    update->GetList().Append(std::move(dict));
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
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::DEV);
  // Have prefs which contain a custom item and hides 1 built-in item
  {
    ListPrefUpdate update(&prefs_, sidebar::kSidebarHiddenBuiltInItems);
    update->ClearList();
    update->Append(static_cast<int>(SidebarItem::BuiltInItemType::kBookmarks));
  }
  {
    ListPrefUpdate update(&prefs_, sidebar::kSidebarItems);
    base::Value::Dict dict;
    dict.Set(sidebar::kSidebarItemURLKey, "https://custom1.brave.com/");
    dict.Set(sidebar::kSidebarItemTitleKey, "Custom Item 1");
    dict.Set(sidebar::kSidebarItemTypeKey,
             static_cast<int>(SidebarItem::Type::kTypeWeb));
    dict.Set(sidebar::kSidebarItemOpenInPanelKey, false);
    update->GetList().Append(std::move(dict));
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
  std::vector<SidebarItem::BuiltInItemType> remaining_default_items{
      SidebarItem::BuiltInItemType::kBraveTalk,
      SidebarItem::BuiltInItemType::kWallet,
      SidebarItem::BuiltInItemType::kReadingList,
  };
  // Get expected indexes (the custom item will replace the index of the removed
  // built-in).
  const auto& all_default_item_types = SidebarService::kDefaultBuiltInItemTypes;
  // There should also be the custom item we added
  EXPECT_EQ(remaining_default_items.size() + 1, service_->items().size());
  for (auto built_in_type : remaining_default_items) {
    auto iter = base::ranges::find(items, built_in_type,
                                   &SidebarItem::built_in_item_type);
    EXPECT_NE(iter, items.end());
    auto expected_index =
        base::ranges::find(all_default_item_types, built_in_type) -
        std::begin(all_default_item_types);
    auto index = iter - items.begin();
    EXPECT_EQ(expected_index, index)
        << "New item with ID " << static_cast<int>(built_in_type)
        << " was inserted at expected index";
  }
}

TEST_F(SidebarServiceTest, MigratePrefSidebarBuiltInItemsSomeHidden) {
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::BETA);
  // Make prefs already have old-style builtin items before service
  // initialization.
  {
    ListPrefUpdate update(&prefs_, sidebar::kSidebarItems);
    update->ClearList();

    base::Value::Dict dict;
    dict.Set(sidebar::kSidebarItemURLKey, "https://anything.brave.com/");
    dict.Set(sidebar::kSidebarItemTitleKey, "Anything");
    dict.Set(sidebar::kSidebarItemTypeKey,
             static_cast<int>(SidebarItem::Type::kTypeBuiltIn));
    dict.Set(sidebar::kSidebarItemBuiltInItemTypeKey,
             static_cast<int>(SidebarItem::BuiltInItemType::kBraveTalk));
    dict.Set(sidebar::kSidebarItemOpenInPanelKey, true);
    update->GetList().Append(std::move(dict));
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
  EXPECT_EQ(2UL, service_->items().size());
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
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::BETA);
  // Make prefs already have ALL old-style builtin items before service
  // initialization. This tests that when not adding anything to the new pref
  // that re-migration will not break anything.
  // Also add a custom item so that the main items pref is not default value.
  {
    ListPrefUpdate update(&prefs_, sidebar::kSidebarItems);
    update->ClearList();

    std::vector<SidebarItem::BuiltInItemType> hideable_types{
        SidebarItem::BuiltInItemType::kBraveTalk,
        SidebarItem::BuiltInItemType::kWallet,
        SidebarItem::BuiltInItemType::kBookmarks,
    };

    for (const auto& built_in_type : hideable_types) {
      base::Value::Dict dict;
      dict.Set(sidebar::kSidebarItemURLKey, "https://anything.brave.com/");
      dict.Set(sidebar::kSidebarItemTitleKey, "Anything");
      dict.Set(sidebar::kSidebarItemTypeKey,
               static_cast<int>(SidebarItem::Type::kTypeBuiltIn));
      dict.Set(sidebar::kSidebarItemBuiltInItemTypeKey,
               static_cast<int>(built_in_type));
      dict.Set(sidebar::kSidebarItemOpenInPanelKey, true);
      update->GetList().Append(std::move(dict));
    }

    base::Value::Dict dict;
    dict.Set(sidebar::kSidebarItemURLKey, "https://custom1.brave.com/");
    dict.Set(sidebar::kSidebarItemTitleKey, "Custom Item 1");
    dict.Set(sidebar::kSidebarItemTypeKey,
             static_cast<int>(SidebarItem::Type::kTypeWeb));
    dict.Set(sidebar::kSidebarItemOpenInPanelKey, false);
    update->GetList().Append(std::move(dict));
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
  EXPECT_EQ(5UL, service_->items().size());
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
  EXPECT_EQ(5UL, service_->items().size());

  // Check again after service updates prefs. Force serialization by performing
  // a move operation (and move it back).
  service_->MoveItem(0, 1);
  service_->MoveItem(0, 1);
  ResetService();
  InitService();
  // Pref now includes new default items added after migration (ReadingList),
  // so size has increased by 1.
  EXPECT_EQ(5UL, preference->GetValue()->GetList().size());
  EXPECT_EQ(0UL, hidden_preference->GetValue()->GetList().size());
  EXPECT_FALSE(hidden_preference->IsDefaultValue());
  EXPECT_FALSE(preference->IsDefaultValue());
  EXPECT_EQ(5UL, service_->items().size());
  // Verify that new a new item not contained in prefs was added at correct
  // index.
  auto items = service_->items();
  auto iter =
      base::ranges::find(items, SidebarItem::BuiltInItemType::kReadingList,
                         &SidebarItem::built_in_item_type);
  auto index = iter - items.begin();
  EXPECT_EQ(3, index);
}

// Verify service has migrated the previous pref format where built-in items
// had url stored and not built-in-item-type.
TEST_F(SidebarServiceTest, MigratePrefSidebarBuiltInItemsNoType) {
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::BETA);
  // Make prefs already have old-style builtin items before service
  // initialization.
  {
    ListPrefUpdate update(&prefs_, sidebar::kSidebarItems);
    update->ClearList();

    // Items should not receive a built-in-item-type.
    std::vector<std::string> urls{
        "https://together.brave.com/",
        "chrome://wallet/",
        "chrome://bookmarks/",
        "chrome://history/",
    };
    for (const auto& url : urls) {
      base::Value::Dict dict;
      dict.Set(sidebar::kSidebarItemURLKey, url);
      dict.Set(sidebar::kSidebarItemTitleKey, "Anything");
      dict.Set(sidebar::kSidebarItemTypeKey,
               static_cast<int>(SidebarItem::Type::kTypeBuiltIn));
      dict.Set(sidebar::kSidebarItemOpenInPanelKey, true);
      update->Append(base::Value(std::move(dict)));
    }
    // Add a custom item to make sure we don't interfere with it
    base::Value::Dict dict;
    dict.Set(sidebar::kSidebarItemURLKey, "chrome://settings/help");
    dict.Set(sidebar::kSidebarItemTitleKey, "Anything");
    dict.Set(sidebar::kSidebarItemTypeKey,
             static_cast<int>(SidebarItem::Type::kTypeWeb));
    dict.Set(sidebar::kSidebarItemOpenInPanelKey, false);
    update->Append(base::Value(std::move(dict)));
  }

  // Not crashing is a good indicator this test has passed
  InitService();

  // Verify migration
  auto& items = prefs_.GetList(kSidebarItems);
  for (const auto& item : items) {
    const auto item_type =
        static_cast<SidebarItem::Type>(*item.FindIntKey(kSidebarItemTypeKey));
    if (item_type == SidebarItem::Type::kTypeBuiltIn) {
      const auto built_in_type = static_cast<SidebarItem::BuiltInItemType>(
          *item.FindIntKey(kSidebarItemBuiltInItemTypeKey));
      EXPECT_NE(built_in_type, SidebarItem::BuiltInItemType::kNone);
    }
  }

  // Verify the expected item count includes all default items (since all are
  // included in the pref, above), minus the obsolete items (history), plus any
  // new default items, plus the custom item.
  EXPECT_EQ(service_->items().size(),
            std::size(SidebarService::kDefaultBuiltInItemTypes) -
                2 /* for history and playlist: invisible built-in itmes */
                + 1 /*for custom item added above*/);
}

TEST_F(SidebarServiceTest, HidesBuiltInItemsViaPref) {
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::BETA);
  // Verify default state
  InitService();
  auto items = service_->items();
  EXPECT_TRUE(base::Contains(items, SidebarItem::BuiltInItemType::kBookmarks,
                             &SidebarItem::built_in_item_type));

  // Update pref to hide bookmarks item
  // Make prefs already have old-style builtin items before service
  // initialization.
  {
    ListPrefUpdate update(&prefs_, sidebar::kSidebarHiddenBuiltInItems);
    update->ClearList();
    update->Append(static_cast<int>(SidebarItem::BuiltInItemType::kBookmarks));
  }

  // Verify new state doesn't include bookmarks item
  InitService();
  items = service_->items();
  EXPECT_FALSE(base::Contains(items, SidebarItem::BuiltInItemType::kBookmarks,
                              &SidebarItem::built_in_item_type));
}

TEST_F(SidebarServiceTest, HidesBuiltInItemsViaService) {
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::BETA);
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
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::BETA);
  // Make prefs already have builtin items before service initialization.
  // And it has old url in old pref format (storing built-in items).
  {
    ListPrefUpdate update(&prefs_, sidebar::kSidebarItems);
    update->ClearList();

    base::Value::Dict dict;
    dict.Set(sidebar::kSidebarItemURLKey, "https://deprecated.brave.com/");
    dict.Set(sidebar::kSidebarItemTitleKey, "Brave together");
    dict.Set(sidebar::kSidebarItemTypeKey,
             static_cast<int>(SidebarItem::Type::kTypeBuiltIn));
    dict.Set(sidebar::kSidebarItemBuiltInItemTypeKey,
             static_cast<int>(SidebarItem::BuiltInItemType::kBraveTalk));
    dict.Set(sidebar::kSidebarItemOpenInPanelKey, true);
    update->GetList().Append(std::move(dict));
  }

  InitService();

  // Check service has updated built-in item. Previously url was deprecated.xxx.
  EXPECT_EQ(2UL, service_->items().size());
  EXPECT_EQ(GURL(kBraveTalkURL), service_->items()[0].url);

  // Simulate re-launch and check service has still updated items.
  ResetService();

  InitService();

  // Check service has updated built-in item. Previously url was deprecated.xxx.
  EXPECT_EQ(2UL, service_->items().size());
  EXPECT_EQ(GURL(kBraveTalkURL), service_->items()[0].url);
}

TEST_F(SidebarServiceTest, BuiltInItemDoesntHaveHistoryItem) {
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::BETA);
  // Make prefs already have builtin items before service initialization.
  // And it has history item.
  {
    ListPrefUpdate update(&prefs_, sidebar::kSidebarItems);
    update->ClearList();

    base::Value::Dict dict;
    dict.Set(sidebar::kSidebarItemURLKey, "https://deprecated.brave.com/");
    dict.Set(sidebar::kSidebarItemTypeKey,
             static_cast<int>(SidebarItem::Type::kTypeBuiltIn));
    dict.Set(sidebar::kSidebarItemBuiltInItemTypeKey,
             static_cast<int>(SidebarItem::BuiltInItemType::kHistory));
    dict.Set(sidebar::kSidebarItemOpenInPanelKey, true);
    update->GetList().Append(std::move(dict));
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
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::STABLE);
  // Show on click is deprecated.
  // Treat it as a show on mouse over.
  prefs_.SetInteger(
      kSidebarShowOption,
      static_cast<int>(SidebarService::ShowSidebarOption::kShowOnClick));

  InitService();
  EXPECT_EQ(SidebarService::ShowSidebarOption::kShowOnMouseOver,
            service_->GetSidebarShowOption());
}

TEST_F(SidebarServiceTest, SidebarShowOptionsDefaultTestStable) {
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::STABLE);
  InitService();
  EXPECT_EQ(SidebarService::ShowSidebarOption::kShowNever,
            service_->GetSidebarShowOption());
}

TEST_F(SidebarServiceTest, SidebarShowOptionsDefaultTestNonStable) {
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::BETA);
  InitService();
  EXPECT_EQ(SidebarService::ShowSidebarOption::kShowAlways,
            service_->GetSidebarShowOption());
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
    if (iter == items.end())
      return;

    const int index = std::distance(items.begin(), iter);
    service_->RemoveItemAt(index);
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

// Check GetDefaultPanelItem() returns valid panel item.
TEST_F(SidebarServiceTestWithPlaylist, GetDefaultPanelItem) {
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::BETA);
  InitService();

  while (SidebarHasDefaultPanelItem()) {
    EXPECT_TRUE(service_->GetDefaultPanelItem());
    RemoveAnySidebarPanelItem();
  }

  EXPECT_FALSE(service_->GetDefaultPanelItem());
}

}  // namespace sidebar
