/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "brave/components/sidebar/constants.h"
#include "brave/components/sidebar/pref_names.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sidebar {

class SidebarServiceTest : public testing::Test,
                           public SidebarService::Observer {
 public:
  SidebarServiceTest() = default;

  ~SidebarServiceTest() override = default;

  void SetUp() override {
    SidebarService::RegisterProfilePrefs(prefs_.registry());
  }

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
  InitService();

  // Check the default items count.
  EXPECT_EQ(3UL, service_->items().size());
  EXPECT_EQ(0UL, service_->GetNotAddedDefaultSidebarItems().size());

  // Cache 1st item to compare after removing.
  const SidebarItem item = service_->items()[0];
  EXPECT_TRUE(IsBuiltInType(item));

  EXPECT_FALSE(on_will_remove_item_called_);
  EXPECT_FALSE(on_item_removed_called_);
  EXPECT_EQ(-1, item_index_on_called_);

  service_->RemoveItemAt(0);
  EXPECT_EQ(2UL, service_->items().size());
  EXPECT_EQ(1UL, service_->GetNotAddedDefaultSidebarItems().size());
  EXPECT_EQ(0, item_index_on_called_);
  EXPECT_TRUE(on_will_remove_item_called_);
  EXPECT_TRUE(on_item_removed_called_);
  ClearState();

  // Add again.
  EXPECT_FALSE(on_item_added_called_);
  service_->AddItem(item);
  // New item will be added at last.
  EXPECT_EQ(2, item_index_on_called_);
  EXPECT_EQ(3UL, service_->items().size());
  EXPECT_EQ(0UL, service_->GetNotAddedDefaultSidebarItems().size());
  EXPECT_TRUE(on_item_added_called_);
  ClearState();

  const SidebarItem item2 = SidebarItem::Create(
      GURL("https://www.brave.com/"), std::u16string(),
      SidebarItem::Type::kTypeWeb, SidebarItem::BuiltInItemType::kNone, true);
  EXPECT_TRUE(IsWebType(item2));
  service_->AddItem(item2);
  EXPECT_EQ(3, item_index_on_called_);
  EXPECT_EQ(4UL, service_->items().size());
  // Default item count is not changed.
  EXPECT_EQ(3UL, service_->GetDefaultSidebarItemsFromCurrentItems().size());
}

TEST_F(SidebarServiceTest, MoveItem) {
  InitService();

  // Add one more item to test with 4 items.
  SidebarItem new_item;
  new_item.url = GURL("https://brave.com");
  service_->AddItem(new_item);
  EXPECT_EQ(4UL, service_->items().size());

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

TEST_F(SidebarServiceTest, BuiltInItemUpdateTestWithBuiltInItemTypeKey) {
  // Make prefs already have builtin items before service initialization.
  // And it has old url.
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
  EXPECT_EQ(1UL, service_->items().size());
  EXPECT_EQ("https://talk.brave.com/widget", service_->items()[0].url);

  // Simulate re-launch and check service has still updated items.
  service_->RemoveObserver(this);
  service_.reset();

  InitService();

  // Check service has updated built-in item. Previously url was deprecated.xxx.
  EXPECT_EQ(1UL, service_->items().size());
  EXPECT_EQ("https://talk.brave.com/widget", service_->items()[0].url);
}

TEST_F(SidebarServiceTest, BuiltInItemDoesntHaveHistoryItem) {
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
  EXPECT_EQ(0UL, service_->items().size());

  // Make sure history is not included in the not added default items list.
  auto not_added_default_items = service_->GetNotAddedDefaultSidebarItems();
  EXPECT_EQ(3UL, not_added_default_items.size());
  for (const auto& item : not_added_default_items) {
    EXPECT_NE(SidebarItem::BuiltInItemType::kHistory, item.built_in_item_type);
  }
}

TEST_F(SidebarServiceTest, BuiltInItemUpdateTestWithoutBuiltInItemTypeKey) {
  // Prepare built-in item in prefs w/o setting BuiltInItemType.
  // If not stored, service uses url to get proper latest properties.
  {
    ListPrefUpdate update(&prefs_, sidebar::kSidebarItems);
    update->ClearList();

    base::Value dict(base::Value::Type::DICTIONARY);
    dict.SetStringKey(sidebar::kSidebarItemURLKey,
                      "https://together.brave.com/");
    dict.SetStringKey(sidebar::kSidebarItemTitleKey, "Brave together");
    dict.SetIntKey(sidebar::kSidebarItemTypeKey,
                   static_cast<int>(SidebarItem::Type::kTypeBuiltIn));
    dict.SetBoolKey(sidebar::kSidebarItemOpenInPanelKey, true);
    update->Append(std::move(dict));
  }

  InitService();

  // Check item type is set properly.
  EXPECT_EQ(1UL, service_->items().size());
  EXPECT_EQ(SidebarItem::BuiltInItemType::kBraveTalk,
            service_->items()[0].built_in_item_type);
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

TEST_F(SidebarServiceTest, SidebarShowOptionsDefaultTest) {
  InitService();
  EXPECT_EQ(SidebarService::ShowSidebarOption::kShowAlways,
            service_->GetSidebarShowOption());
}

}  // namespace sidebar
