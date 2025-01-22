// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/task/current_thread.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {
class TabDataObserver : public mojom::TabDataObserver {
 public:
  TabDataObserver() = default;
  ~TabDataObserver() override = default;

  TabDataObserver(const TabDataObserver&) = delete;
  TabDataObserver& operator=(const TabDataObserver&) = delete;

  TabDataObserver(TabDataObserver&&) = default;
  TabDataObserver& operator=(TabDataObserver&&) = default;

  // mojom::TabDataObserver
  void TabDataChanged(std::vector<mojom::TabDataPtr> tabs) override {
    this->last_tabs_ = std::move(tabs);
  }

  const std::vector<mojom::TabDataPtr>& last_tabs() const { return last_tabs_; }

 private:
  std::vector<mojom::TabDataPtr> last_tabs_;
};
}  // namespace

class TabTrackerServiceTest : public testing::Test {
 public:
  TabTrackerServiceTest() {
    mojo::PendingRemote<mojom::TabDataObserver> pending_remote;
    auto observer = std::make_unique<TabDataObserver>();
    observer_ = observer.get();

    auto receiver = mojo::MakeSelfOwnedReceiver(
        std::move(observer), pending_remote.InitWithNewPipeAndPassReceiver());
    service_.AddObserver(std::move(pending_remote));
  }
  ~TabTrackerServiceTest() override = default;

  const std::vector<mojom::TabDataPtr>& last_tabs() const {
    return observer_->last_tabs();
  }

  void UpdateTab(mojom::TabDataPtr tab) {
    auto id = tab->id;
    service_.UpdateTab(id, std::move(tab));
  }

  void DeleteTab(int id) { service_.UpdateTab(id, nullptr); }

 private:
  base::test::SingleThreadTaskEnvironment task_environment_;

  TabTrackerService service_;
  raw_ptr<TabDataObserver> observer_;
};

TEST_F(TabTrackerServiceTest, TabsCanBeAdded) {
  auto tab1 = mojom::TabData::New();
  tab1->title = "One";
  tab1->url = GURL("https://one.com");
  tab1->id = 1;
  UpdateTab(std::move(tab1));

  auto tab2 = mojom::TabData::New();
  tab2->title = "Two";
  tab2->url = GURL("https://two.com");
  tab2->id = 2;
  UpdateTab(std::move(tab2));

  base::test::RunUntil([&]() { return last_tabs().size() == 2; });

  EXPECT_EQ(last_tabs()[0]->title, "One");
  EXPECT_EQ(last_tabs()[0]->url, GURL("https://one.com"));
  EXPECT_EQ(last_tabs()[0]->id, 1);

  EXPECT_EQ(last_tabs()[1]->title, "Two");
  EXPECT_EQ(last_tabs()[1]->url, GURL("https://two.com"));
  EXPECT_EQ(last_tabs()[1]->id, 2);
}

TEST_F(TabTrackerServiceTest, TabsCanBeRemoved) {
  auto tab1 = mojom::TabData::New();
  tab1->title = "One";
  tab1->url = GURL("https://one.com");
  tab1->id = 1;
  UpdateTab(std::move(tab1));

  auto tab2 = mojom::TabData::New();
  tab2->title = "Two";
  tab2->url = GURL("https://two.com");
  tab2->id = 2;
  UpdateTab(std::move(tab2));

  DeleteTab(1);

  base::test::RunUntil([&]() { return last_tabs().size() == 1; });
  EXPECT_EQ(last_tabs()[0]->title, "Two");
  EXPECT_EQ(last_tabs()[0]->url, GURL("https://two.com"));
  EXPECT_EQ(last_tabs()[0]->id, 2);
}

TEST_F(TabTrackerServiceTest, TabsCanBeUpdated) {
  auto tab1 = mojom::TabData::New();
  tab1->title = "One";
  tab1->url = GURL("https://one.com");
  tab1->id = 1;
  UpdateTab(tab1->Clone());

  auto tab2 = mojom::TabData::New();
  tab2->title = "Two";
  tab2->url = GURL("https://two.com");
  tab2->id = 2;
  UpdateTab(std::move(tab2));

  tab1->title = "One Updated";
  tab1->url = GURL("https://one-updated.com");
  UpdateTab(tab1->Clone());

  base::test::RunUntil([&]() { return last_tabs().size() == 2; });

  EXPECT_EQ(last_tabs()[0]->title, "One Updated");
  EXPECT_EQ(last_tabs()[0]->url, GURL("https://one-updated.com"));
  EXPECT_EQ(last_tabs()[0]->id, 1);

  EXPECT_EQ(last_tabs()[1]->title, "Two");
  EXPECT_EQ(last_tabs()[1]->url, GURL("https://two.com"));
  EXPECT_EQ(last_tabs()[1]->id, 2);
}

TEST_F(TabTrackerServiceTest, DeletingNonExistentTabDoesNothing) {
  auto tab1 = mojom::TabData::New();
  tab1->title = "One";
  tab1->url = GURL("https://one.com");
  tab1->id = 1;
  UpdateTab(tab1->Clone());

  DeleteTab(2);

  base::test::RunUntil([&]() { return last_tabs().size() == 1; });
}

TEST_F(TabTrackerServiceTest, InvalidSchemesAreNotTracked) {
  auto internal = mojom::TabData::New();
  internal->title = "Internal Page";
  internal->url = GURL("chrome://page");
  internal->id = 1;
  UpdateTab(std::move(internal));

  auto internal_untrusted = mojom::TabData::New();
  internal_untrusted->title = "Internal Untrusted Page";
  internal_untrusted->url = GURL("chrome-untrusted://page");
  internal_untrusted->id = 2;
  UpdateTab(std::move(internal_untrusted));

  auto normal = mojom::TabData::New();
  normal->title = "Normal";
  normal->url = GURL("https://normal.com");
  normal->id = 3;
  UpdateTab(std::move(normal));

  base::test::RunUntil([&]() { return last_tabs().size() == 1; });

  EXPECT_EQ(last_tabs()[0]->title, "Normal");
  EXPECT_EQ(last_tabs()[0]->url, GURL("https://normal.com"));
  EXPECT_EQ(last_tabs()[0]->id, 3);
}

TEST_F(TabTrackerServiceTest, NavigationToInvalidSchemeUntracksTab) {
  auto page = mojom::TabData::New();
  page->title = "Page";
  page->url = GURL("https://page.com");
  page->id = 1;
  UpdateTab(page.Clone());

  base::test::RunUntil([&]() { return last_tabs().size() == 1; });

  page->url = GURL("chrome://page");
  UpdateTab(page->Clone());

  base::test::RunUntil([&]() { return last_tabs().size() == 0; });
}

TEST_F(TabTrackerServiceTest, NavigationToValidSchemeTracksTab) {
  auto page = mojom::TabData::New();
  page->title = "Page";
  page->url = GURL("chrome://page");
  page->id = 1;
  UpdateTab(page.Clone());

  base::test::RunUntil([&]() { return last_tabs().size() == 0; });

  page->url = GURL("https://page.com");
  UpdateTab(page->Clone());

  base::test::RunUntil([&]() { return last_tabs().size() == 1; });

  EXPECT_EQ(last_tabs()[0]->url, GURL("https://page.com"));
}

TEST_F(TabTrackerServiceTest, DeleteInvalidSchemeTabDoesNothing) {
  auto page = mojom::TabData::New();
  page->title = "Page";
  page->url = GURL("chrome://page");
  page->id = 1;
  UpdateTab(page.Clone());

  base::test::RunUntil([&]() { return last_tabs().size() == 0; });

  DeleteTab(1);

  base::test::RunUntil([&]() { return last_tabs().size() == 0; });
}

}  // namespace ai_chat
