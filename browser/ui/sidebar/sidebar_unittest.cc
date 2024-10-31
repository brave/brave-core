/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/sidebar/browser/constants.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/version_info/channel.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using ::testing::Eq;
using ::testing::Optional;

namespace sidebar {

class MockSidebarModelObserver : public SidebarModel::Observer {
 public:
  MockSidebarModelObserver() = default;
  ~MockSidebarModelObserver() override = default;

  MOCK_METHOD(void,
              OnItemAdded,
              (const SidebarItem& item, size_t index, bool user_gesture),
              (override));
  MOCK_METHOD(void,
              OnItemMoved,
              (const SidebarItem& item, size_t from, size_t to),
              (override));
  MOCK_METHOD(void, OnItemRemoved, (size_t index), (override));
  MOCK_METHOD(void,
              OnActiveIndexChanged,
              (std::optional<size_t> old_index,
               std::optional<size_t> new_index),
              (override));
  MOCK_METHOD(void,
              OnItemUpdated,
              (const SidebarItem& item, const SidebarItemUpdate& update),
              (override));
  MOCK_METHOD(void,
              OnFaviconUpdatedForItem,
              (const SidebarItem& item, const gfx::ImageSkia& image),
              (override));
};

class SidebarModelTest : public testing::Test {
 public:
  SidebarModelTest() = default;

  ~SidebarModelTest() override = default;

  void SetUp() override {
    // Instantiate SidebarServiceFactory before creating TestingProfile
    // as SidebarServiceFactory registers profile prefs.
    SidebarServiceFactory::GetInstance();
    profile_ = std::make_unique<TestingProfile>();
    service_ = SidebarServiceFactory::GetForProfile(profile_.get());
    model_ = std::make_unique<SidebarModel>(profile_.get());
    observation_.Observe(model_.get());
  }

  Profile* profile() { return profile_.get(); }
  SidebarModel* model() { return model_.get(); }
  SidebarService* service() { return service_; }

  content::BrowserTaskEnvironment browser_task_environment_;
  testing::NiceMock<MockSidebarModelObserver> observer_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<SidebarModel> model_;
  base::ScopedObservation<SidebarModel, SidebarModel::Observer> observation_{
      &observer_};
  raw_ptr<SidebarService> service_ = nullptr;
};

TEST_F(SidebarModelTest, ItemsChangedTest) {
  auto built_in_items_size = service()->items().size();

  model()->Init(nullptr);

  EXPECT_THAT(model()->active_index(), Eq(std::nullopt));

  // Add one more item to test with 5 items.
  SidebarItem new_item = SidebarItem::Create(
      GURL("https://www.brave.com/"), u"brave software",
      SidebarItem::Type::kTypeWeb, SidebarItem::BuiltInItemType::kNone, false);

  service()->AddItem(new_item);
  const auto items_count = built_in_items_size + 1;
  EXPECT_EQ(items_count, service()->items().size());

  // Update last item w/ url change.
  SidebarItemUpdate expected_update{(items_count - 1), false, true};
  EXPECT_CALL(observer_, OnItemUpdated(testing::_, expected_update)).Times(1);
  service()->UpdateItem(GURL("https://www.brave.com/"),
                        GURL("https://brave.com/"), u"brave software",
                        u"brave software");
  testing::Mock::VerifyAndClearExpectations(&observer_);

  // Update last item w/o url change.
  expected_update.url_updated = false;
  expected_update.title_updated = true;
  EXPECT_CALL(observer_, OnItemUpdated(testing::_, expected_update)).Times(1);
  service()->UpdateItem(GURL("https://brave.com/"), GURL("https://brave.com/"),
                        u"brave software", u"brave");
  testing::Mock::VerifyAndClearExpectations(&observer_);

  // Move item at 1 to at index 2.
  // Total size and active index is not changed when currently active index is
  // -1.
  const size_t items_size = service()->items().size();
  // Cache data at index 1.
  const auto item_data = service()->items()[1];

  EXPECT_CALL(observer_, OnItemMoved(testing::_, 1, 2)).Times(1);
  EXPECT_CALL(observer_, OnActiveIndexChanged(testing::_, testing::_)).Times(0);
  service()->MoveItem(1, 2);
  testing::Mock::VerifyAndClearExpectations(&observer_);

  EXPECT_EQ(item_data.built_in_item_type,
            service()->items()[2].built_in_item_type);
  EXPECT_EQ(item_data.url, service()->items()[2].url);
  EXPECT_EQ(item_data.title, service()->items()[2].title);
  EXPECT_THAT(model()->active_index(), Eq(std::nullopt));
  EXPECT_EQ(items_size, service()->items().size());

  model()->SetActiveIndex(1);
  EXPECT_THAT(model()->active_index(), Optional(1u));

  // Move item at 1 to 2. This causes active index change because item at 1 was
  // active item. After moving, active item index should be 2.
  EXPECT_CALL(observer_, OnItemMoved(testing::_, 1, 2)).Times(1);
  EXPECT_CALL(observer_, OnActiveIndexChanged(Optional(1), Optional(2)))
      .Times(1);
  service()->MoveItem(1, 2);
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_THAT(model()->active_index(), Optional(2u));

  // Moving item from 1 to 0 doesn't affect active index.
  EXPECT_CALL(observer_, OnItemMoved(testing::_, 1, 0)).Times(1);
  EXPECT_CALL(observer_, OnActiveIndexChanged(testing::_, testing::_)).Times(0);
  service()->MoveItem(1, 0);
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_THAT(model()->active_index(), Optional(2u));

  // Moving item from 3 to 0 affect active index. Items behind the active
  // item(at 2) to the front of active index. So, active item is also moved from
  // 2 to 3 index.
  EXPECT_CALL(observer_, OnItemMoved(testing::_, 3, 0)).Times(1);
  EXPECT_CALL(observer_, OnActiveIndexChanged(Optional(2), Optional(3)))
      .Times(1);
  service()->MoveItem(3, 0);
  testing::Mock::VerifyAndClearExpectations(&observer_);
  EXPECT_THAT(model()->active_index(), Optional(3u));
}

TEST_F(SidebarModelTest, CanUseNotAddedBuiltInItemInsteadOfTest) {
  GURL talk("https://talk.brave.com/1Ar1vHfLBWX2sAdi");
  // False because builtin talk item is already added.
  EXPECT_FALSE(HiddenDefaultSidebarItemsContains(service(), talk));

  // Remove builtin talk item and check builtin talk item will be used
  // instead of adding |talk| url.
  const auto items = service()->items();
  const auto talk_iter =
      base::ranges::find(items, SidebarItem::BuiltInItemType::kBraveTalk,
                         &SidebarItem::built_in_item_type);
  ASSERT_NE(talk_iter, items.cend());
  service()->RemoveItemAt(std::distance(items.cbegin(), talk_iter));
  EXPECT_TRUE(HiddenDefaultSidebarItemsContains(service(), talk));
}

TEST_F(SidebarModelTest, ActiveIndexChangedAfterItemAdded) {
  model()->SetActiveIndex(1);
  EXPECT_THAT(model()->active_index(), Optional(1u));

  SidebarItem item_1 = SidebarItem::Create(
      GURL("https://www.brave.com/"), u"brave software",
      SidebarItem::Type::kTypeWeb, SidebarItem::BuiltInItemType::kNone, false);

  // Check active index is still 1 when new item is added at 2.
  model()->AddItem(item_1, 2, true);
  EXPECT_THAT(model()->active_index(), Optional(1u));

  SidebarItem item_2 = SidebarItem::Create(
      GURL("https://www.braves.com/"), u"brave software",
      SidebarItem::Type::kTypeWeb, SidebarItem::BuiltInItemType::kNone, false);

  // Check active index is changed to 2 when new item is added at 1.
  model()->AddItem(item_2, 1, true);
  EXPECT_THAT(model()->active_index(), Optional(2u));
}

// Check Leo item is top-most item.
TEST_F(SidebarModelTest, TopItemTest) {
  const auto first_item = service()->items()[0];
  EXPECT_EQ(first_item.built_in_item_type,
            SidebarItem::BuiltInItemType::kChatUI);
}

TEST(SidebarUtilTest, SidebarShowOptionsDefaultTest) {
  EXPECT_EQ(SidebarService::ShowSidebarOption::kShowNever,
            GetDefaultShowSidebarOption(version_info::Channel::STABLE));
  EXPECT_EQ(SidebarService::ShowSidebarOption::kShowAlways,
            GetDefaultShowSidebarOption(version_info::Channel::BETA));
  EXPECT_EQ(SidebarService::ShowSidebarOption::kShowAlways,
            GetDefaultShowSidebarOption(version_info::Channel::CANARY));
}

TEST(SidebarUtilTest, ConvertURLToBuiltInItemURLTest) {
  EXPECT_EQ(GURL(kBraveTalkURL),
            ConvertURLToBuiltInItemURL(GURL("https://talk.brave.com")));
  EXPECT_EQ(GURL(kBraveTalkURL),
            ConvertURLToBuiltInItemURL(
                GURL("https://talk.brave.com/1Ar1vHfLBWX2sAdi")));
  EXPECT_EQ(
      GURL(kBraveUIWalletPageURL),
      ConvertURLToBuiltInItemURL(GURL("chrome://wallet/crypto/onboarding")));

  // Not converted for url that doesn't relavant builtin item.
  GURL brave_com("https://www.brave.com/");
  EXPECT_EQ(brave_com, ConvertURLToBuiltInItemURL(brave_com));
}

}  // namespace sidebar
