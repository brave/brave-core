/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <memory>
#include <utility>

#include "brave/components/sidebar/constants.h"
#include "brave/components/sidebar/pref_names.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "components/version_info/channel.h"
#include "testing/gtest/include/gtest/gtest.h"

using version_info::Channel;

namespace sidebar {

class SidebarServiceTest : public testing::Test,
                           public SidebarService::Observer {
 public:
  SidebarServiceTest() = default;

  ~SidebarServiceTest() override = default;

  void TearDown() override { service_->RemoveObserver(this); }

  void InitService() {
    service_.reset(new SidebarService(&prefs_));
    service_->AddObserver(this);
  }

  // SidebarServiceObserver overrides:
  void OnItemAdded(const SidebarItem& item, int index) override {
    item_index_on_called_ = index;
    on_item_added_called_ = true;
  }

  void OnWillRemoveItem(const SidebarItem& item, int index) override {
    item_index_on_called_ = index;
    on_will_remove_item_called_ = true;
  }

  void OnItemRemoved(const SidebarItem& item, int index) override {
    // Make sure OnWillRemoveItem() must be called before this.
    EXPECT_TRUE(on_will_remove_item_called_);
    item_index_on_called_ = index;
    on_item_removed_called_ = true;
  }

  void OnItemMoved(const SidebarItem& item, int from, int to) override {
    on_item_moved_called_ = true;
  }

  void ClearState() {
    item_index_on_called_ = -1;
    on_item_added_called_ = false;
    on_will_remove_item_called_ = false;
    on_item_removed_called_ = false;
    on_item_moved_called_ = false;
  }

  int item_index_on_called_ = -1;
  bool on_item_added_called_ = false;
  bool on_will_remove_item_called_ = false;
  bool on_item_removed_called_ = false;
  bool on_item_moved_called_ = false;

  TestingPrefServiceSimple prefs_;
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

  EXPECT_FALSE(on_will_remove_item_called_);
  EXPECT_FALSE(on_item_removed_called_);
  EXPECT_EQ(-1, item_index_on_called_);

  service_->RemoveItemAt(0);
  EXPECT_EQ(3UL, service_->items().size());
  EXPECT_EQ(1UL, service_->GetHiddenDefaultSidebarItems().size());
  EXPECT_EQ(0, item_index_on_called_);
  EXPECT_TRUE(on_will_remove_item_called_);
  EXPECT_TRUE(on_item_removed_called_);
  ClearState();

  // Add again.
  EXPECT_FALSE(on_item_added_called_);
  service_->AddItem(item);
  // New item will be added at last.
  EXPECT_EQ(3, item_index_on_called_);
  EXPECT_EQ(4UL, service_->items().size());
  EXPECT_EQ(0UL, service_->GetHiddenDefaultSidebarItems().size());
  EXPECT_TRUE(on_item_added_called_);
  ClearState();

  const SidebarItem item2 = SidebarItem::Create(
      GURL("https://www.brave.com/"), std::u16string(),
      SidebarItem::Type::kTypeWeb, SidebarItem::BuiltInItemType::kNone, true);
  EXPECT_TRUE(IsWebType(item2));
  service_->AddItem(item2);
  EXPECT_EQ(4, item_index_on_called_);
  EXPECT_EQ(5UL, service_->items().size());
  // Default item count is not changed.
  EXPECT_EQ(4UL, service_->GetDefaultSidebarItemsFromCurrentItems().size());
}

TEST_F(SidebarServiceTest, MoveItem) {
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::DEV);
  InitService();

  // Add one more item to test with 4 items.
  SidebarItem new_item;
  new_item.url = GURL("https://brave.com");
  service_->AddItem(new_item);
  EXPECT_EQ(5UL, service_->items().size());

  // Move item at 0 to index 2.
  SidebarItem item = service_->items()[0];
  service_->MoveItem(0, 2);
  EXPECT_EQ(item.url, service_->items()[2].url);
  EXPECT_TRUE(on_item_moved_called_);

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

TEST_F(SidebarServiceTest, MoveItemSavedToPrefs) {
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::DEV);
  InitService();

  // Add one more item to test with 4 items.
  SidebarItem new_item;
  new_item.url = GURL("https://brave.com");
  service_->AddItem(new_item);
  EXPECT_EQ(5UL, service_->items().size());

  // Move item at 0 to index 2.
  SidebarItem item = service_->items()[0];
  GURL url = item.url;
  service_->MoveItem(0, 2);
  EXPECT_TRUE(on_item_moved_called_);
  service_.reset();
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
    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetStringKey(sidebar::kSidebarItemURLKey,
                      "https://custom1.brave.com/");
    dict.SetStringKey(sidebar::kSidebarItemTitleKey, "Custom Item 1");
    dict.SetIntKey(sidebar::kSidebarItemTypeKey,
                   static_cast<int>(SidebarItem::Type::kTypeWeb));
    dict.SetBoolKey(sidebar::kSidebarItemOpenInPanelKey, false);
    update->Append(std::move(dict));
  }

  InitService();
  // None of the items should be the hidden one
  auto items = service_->items();
  auto bookmark_iter =
      std::find_if(items.begin(), items.end(), [](const auto& item) {
        return (item.built_in_item_type ==
                SidebarItem::BuiltInItemType::kBookmarks);
      });
  EXPECT_EQ(bookmark_iter, items.end());
  // Check serialization also perists that
  service_.reset();
  InitService();
  items = service_->items();
  bookmark_iter =
      std::find_if(items.begin(), items.end(), [](const auto& item) {
        return (item.built_in_item_type ==
                SidebarItem::BuiltInItemType::kBookmarks);
      });
  EXPECT_EQ(bookmark_iter, items.end());
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
    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetStringKey(sidebar::kSidebarItemURLKey,
                      "https://custom1.brave.com/");
    dict.SetStringKey(sidebar::kSidebarItemTitleKey, "Custom Item 1");
    dict.SetIntKey(sidebar::kSidebarItemTypeKey,
                   static_cast<int>(SidebarItem::Type::kTypeWeb));
    dict.SetBoolKey(sidebar::kSidebarItemOpenInPanelKey, false);
    update->Append(std::move(dict));
  }

  InitService();
  // Make sure other default items are present. They are considered not seen yet
  // since kSidebarItems was not default value and did not contain them.
  // None of the items should be the hidden one
  auto items = service_->items();
  auto bookmark_iter =
      std::find_if(items.begin(), items.end(), [](const auto& item) {
        return (item.built_in_item_type ==
                SidebarItem::BuiltInItemType::kBookmarks);
      });
  EXPECT_EQ(bookmark_iter, items.end());
  // All other default items should be present even though not present
  // in kSidebarItems pref.
  std::vector<SidebarItem::BuiltInItemType> remaining_default_items{
      SidebarItem::BuiltInItemType::kBraveTalk,
      SidebarItem::BuiltInItemType::kWallet,
      SidebarItem::BuiltInItemType::kReadingList,
  };
  // Get expected indexes (the custom item will replace the index of the removed
  // built-in).
  auto all_default_item_types =
      SidebarService::GetDefaultBuiltInItemTypes_ForTesting();
  // There should also be the custom item we added
  EXPECT_EQ(remaining_default_items.size() + 1, service_->items().size());
  for (auto built_in_type : remaining_default_items) {
    auto iter = std::find_if(
        items.begin(), items.end(), [built_in_type](const auto& item) {
          return (item.built_in_item_type == built_in_type);
        });
    EXPECT_NE(iter, items.end());
    auto expected_index =
        std::find(all_default_item_types.begin(), all_default_item_types.end(),
                  built_in_type) -
        all_default_item_types.begin();
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

    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetStringKey(sidebar::kSidebarItemURLKey,
                      "https://anything.brave.com/");
    dict.SetStringKey(sidebar::kSidebarItemTitleKey, "Anything");
    dict.SetIntKey(sidebar::kSidebarItemTypeKey,
                   static_cast<int>(SidebarItem::Type::kTypeBuiltIn));
    dict.SetIntKey(sidebar::kSidebarItemBuiltInItemTypeKey,
                   static_cast<int>(SidebarItem::BuiltInItemType::kBraveTalk));
    dict.SetBoolKey(sidebar::kSidebarItemOpenInPanelKey, true);
    update->Append(std::move(dict));
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
      std::find_if(items.begin(), items.end(), [](const auto& item) {
        return (item.built_in_item_type ==
                SidebarItem::BuiltInItemType::kBraveTalk);
      });
  auto reading_iter =
      std::find_if(items.begin(), items.end(), [](const auto& item) {
        return (item.built_in_item_type ==
                SidebarItem::BuiltInItemType::kReadingList);
      });
  EXPECT_NE(talk_iter, items.end());
  EXPECT_NE(reading_iter, items.end());
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
      base::Value dict(base::Value::Type::DICTIONARY);
      dict.SetStringKey(sidebar::kSidebarItemURLKey,
                        "https://anything.brave.com/");
      dict.SetStringKey(sidebar::kSidebarItemTitleKey, "Anything");
      dict.SetIntKey(sidebar::kSidebarItemTypeKey,
                     static_cast<int>(SidebarItem::Type::kTypeBuiltIn));
      dict.SetIntKey(sidebar::kSidebarItemBuiltInItemTypeKey,
                     static_cast<int>(built_in_type));
      dict.SetBoolKey(sidebar::kSidebarItemOpenInPanelKey, true);
      update->Append(std::move(dict));
    }

    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetStringKey(sidebar::kSidebarItemURLKey,
                      "https://custom1.brave.com/");
    dict.SetStringKey(sidebar::kSidebarItemTitleKey, "Custom Item 1");
    dict.SetIntKey(sidebar::kSidebarItemTypeKey,
                   static_cast<int>(SidebarItem::Type::kTypeWeb));
    dict.SetBoolKey(sidebar::kSidebarItemOpenInPanelKey, false);
    update->Append(std::move(dict));
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
  service_.reset();
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
  service_.reset();
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
  auto iter = std::find_if(items.begin(), items.end(), [](const auto& item) {
    return item.built_in_item_type ==
           SidebarItem::BuiltInItemType::kReadingList;
  });
  auto index = iter - items.begin();
  EXPECT_EQ(3, index);
}

TEST_F(SidebarServiceTest, HidesBuiltInItemsViaPref) {
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::BETA);
  // Verify default state
  InitService();
  auto items = service_->items();
  auto bookmarks_iter =
      std::find_if(items.begin(), items.end(), [](const auto& item) {
        return (item.built_in_item_type ==
                SidebarItem::BuiltInItemType::kBookmarks);
      });
  EXPECT_NE(bookmarks_iter, items.end());

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
  auto bookmarks_iter_removed =
      std::find_if(items.begin(), items.end(), [](const auto& item) {
        return (item.built_in_item_type ==
                SidebarItem::BuiltInItemType::kBookmarks);
      });
  EXPECT_EQ(bookmarks_iter_removed, items.end());
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
  service_.reset();
  InitService();
  items = service_->items();
  auto bookmarks_iter_removed =
      std::find_if(items.begin(), items.end(), [](const auto& item) {
        return (item.built_in_item_type ==
                SidebarItem::BuiltInItemType::kBookmarks);
      });
  EXPECT_EQ(bookmarks_iter_removed, items.end());
}

TEST_F(SidebarServiceTest, BuiltInItemUpdateTestWithBuiltInItemTypeKey) {
  SidebarService::RegisterProfilePrefs(prefs_.registry(), Channel::BETA);
  // Make prefs already have builtin items before service initialization.
  // And it has old url in old pref format (storing built-in items).
  {
    ListPrefUpdate update(&prefs_, sidebar::kSidebarItems);
    update->ClearList();

    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetStringKey(sidebar::kSidebarItemURLKey,
                      "https://deprecated.brave.com/");
    dict.SetStringKey(sidebar::kSidebarItemTitleKey, "Brave together");
    dict.SetIntKey(sidebar::kSidebarItemTypeKey,
                   static_cast<int>(SidebarItem::Type::kTypeBuiltIn));
    dict.SetIntKey(sidebar::kSidebarItemBuiltInItemTypeKey,
                   static_cast<int>(SidebarItem::BuiltInItemType::kBraveTalk));
    dict.SetBoolKey(sidebar::kSidebarItemOpenInPanelKey, true);
    update->Append(std::move(dict));
  }

  InitService();

  // Check service has updated built-in item. Previously url was deprecated.xxx.
  EXPECT_EQ(2UL, service_->items().size());
  EXPECT_EQ(GURL(kBraveTalkURL), service_->items()[0].url);

  // Simulate re-launch and check service has still updated items.
  service_->RemoveObserver(this);
  service_.reset();

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

    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetStringKey(sidebar::kSidebarItemURLKey,
                      "https://deprecated.brave.com/");
    dict.SetIntKey(sidebar::kSidebarItemTypeKey,
                   static_cast<int>(SidebarItem::Type::kTypeBuiltIn));
    dict.SetIntKey(sidebar::kSidebarItemBuiltInItemTypeKey,
                   static_cast<int>(SidebarItem::BuiltInItemType::kHistory));
    dict.SetBoolKey(sidebar::kSidebarItemOpenInPanelKey, true);
    update->Append(std::move(dict));
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

}  // namespace sidebar
