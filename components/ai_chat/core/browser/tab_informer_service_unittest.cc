// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tab_informer_service.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/task/current_thread.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/common/mojom/tab_informer.mojom.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {
class TabListener : public mojom::TabListener {
 public:
  TabListener() = default;
  ~TabListener() override = default;

  TabListener(const TabListener&) = delete;
  TabListener& operator=(const TabListener&) = delete;

  TabListener(TabListener&&) = default;
  TabListener& operator=(TabListener&&) = default;

  // mojom::TabListener
  void TabsChanged(std::vector<mojom::TabPtr> tabs) override {
    this->last_tabs_ = std::move(tabs);
  }

  const std::vector<mojom::TabPtr>& last_tabs() const { return last_tabs_; }

 private:
  std::vector<mojom::TabPtr> last_tabs_;
};
}  // namespace

class TabInformerServiceTest : public testing::Test {
 public:
  TabInformerServiceTest() {
    mojo::PendingRemote<mojom::TabListener> pending_remote;
    auto listener = std::make_unique<TabListener>();
    listener_ = listener.get();

    auto receiver = mojo::MakeSelfOwnedReceiver(
        std::move(listener), pending_remote.InitWithNewPipeAndPassReceiver());
    service_.AddListener(std::move(pending_remote));
  }
  ~TabInformerServiceTest() override = default;

  const std::vector<mojom::TabPtr>& last_tabs() const {
    return listener_->last_tabs();
  }

  void UpdateTab(mojom::TabPtr tab) {
    auto id = tab->id;
    service_.UpdateTab(id, std::move(tab));
  }

  void DeleteTab(int id) { service_.UpdateTab(id, nullptr); }

 private:
  base::test::SingleThreadTaskEnvironment task_environment_;

  TabInformerService service_;
  raw_ptr<TabListener> listener_;
};

TEST_F(TabInformerServiceTest, TabsCanBeAdded) {
  mojom::TabPtr tab1 = mojom::Tab::New();
  tab1->title = "One";
  tab1->url = GURL("https://one.com");
  tab1->id = 1;
  UpdateTab(std::move(tab1));

  mojom::TabPtr tab2 = mojom::Tab::New();
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

TEST_F(TabInformerServiceTest, TabsCanBeRemoved) {
  mojom::TabPtr tab1 = mojom::Tab::New();
  tab1->title = "One";
  tab1->url = GURL("https://one.com");
  tab1->id = 1;
  UpdateTab(std::move(tab1));

  mojom::TabPtr tab2 = mojom::Tab::New();
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

TEST_F(TabInformerServiceTest, TabsCanBeUpdated) {
  mojom::TabPtr tab1 = mojom::Tab::New();
  tab1->title = "One";
  tab1->url = GURL("https://one.com");
  tab1->id = 1;
  UpdateTab(tab1->Clone());

  mojom::TabPtr tab2 = mojom::Tab::New();
  tab2->title = "Two";
  tab2->url = GURL("https://two.com");
  tab2->id = 2;
  UpdateTab(std::move(tab2));

  tab1->title = "One Updated";
  UpdateTab(tab1->Clone());

  base::test::RunUntil([&]() { return last_tabs().size() == 2; });

  EXPECT_EQ(last_tabs()[0]->title, "One Updated");
  EXPECT_EQ(last_tabs()[0]->url, GURL("https://one.com"));
  EXPECT_EQ(last_tabs()[0]->id, 1);

  EXPECT_EQ(last_tabs()[1]->title, "Two");
  EXPECT_EQ(last_tabs()[1]->url, GURL("https://two.com"));
  EXPECT_EQ(last_tabs()[1]->id, 2);
}

}  // namespace ai_chat
