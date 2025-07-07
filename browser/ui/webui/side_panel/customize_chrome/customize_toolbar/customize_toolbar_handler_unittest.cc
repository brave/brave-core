// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/customize_toolbar_handler.h"

#include "base/functional/bind.h"
#include "brave/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/brave_action.h"
#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_toolbar/customize_toolbar.mojom.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_contents_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

class MockPage
    : public side_panel::customize_chrome::mojom::CustomizeToolbarClient {
 public:
  MockPage() = default;
  ~MockPage() override = default;

  mojo::PendingRemote<
      side_panel::customize_chrome::mojom::CustomizeToolbarClient>
  BindAndGetRemote() {
    DCHECK(!receiver_.is_bound());
    return receiver_.BindNewPipeAndPassRemote();
  }

  MOCK_METHOD(void,
              SetActionPinned,
              (side_panel::customize_chrome::mojom::ActionId action_id,
               bool pinned));
  MOCK_METHOD(void, NotifyActionsUpdated, ());

  mojo::Receiver<side_panel::customize_chrome::mojom::CustomizeToolbarClient>
      receiver_{this};
};

}  // namespace

namespace customize_chrome {

class CustomizeToolbarHandlerUnitTest : public testing::Test {
 public:
  CustomizeToolbarHandlerUnitTest() = default;
  ~CustomizeToolbarHandlerUnitTest() override = default;

  // testing::Test:
  void SetUp() override {
    web_contents_ =
        test_web_contents_factory_.CreateWebContents(&testing_profile_);

    handler_ = std::make_unique<CustomizeToolbarHandler>(
        mojo::PendingReceiver<
            side_panel::customize_chrome::mojom::CustomizeToolbarHandler>(),
        mock_page_.BindAndGetRemote(), web_contents_);
  }

  void TearDown() override {
    handler_.reset();
    web_contents_ = nullptr;
  }

 protected:
  sync_preferences::TestingPrefServiceSyncable* GetTestingPrefService() {
    return testing_profile_.GetTestingPrefService();
  }

  content::BrowserTaskEnvironment task_environment_;
  TestingProfile testing_profile_;

  testing::NiceMock<MockPage> mock_page_;
  content::TestWebContentsFactory test_web_contents_factory_;
  raw_ptr<content::WebContents> web_contents_;
  std::unique_ptr<CustomizeToolbarHandler> handler_;
};

TEST_F(CustomizeToolbarHandlerUnitTest,
       OnBraveActionPinnedChanged_ShouldBeCalledWhenPrefsChanged) {
  for (const auto& [id, brave_action] : kBraveActions) {
    const bool pinned =
        GetTestingPrefService()->GetBoolean(brave_action->pref_name);
    EXPECT_CALL(mock_page_, SetActionPinned(id, !pinned));
    GetTestingPrefService()->SetBoolean(brave_action->pref_name, !pinned);
  }
}

TEST_F(CustomizeToolbarHandlerUnitTest, YourChromeLabelShouldBeBraveMenu) {
  handler_->ListCategories(base::BindOnce(
      [](std::vector<side_panel::customize_chrome::mojom::CategoryPtr>
             categories) {
        auto your_chrome_it = std::ranges::find(
            categories,
            side_panel::customize_chrome::mojom::CategoryId::kYourChrome,
            [](const auto& category) { return category->id; });
        ASSERT_NE(your_chrome_it, categories.end());
        EXPECT_EQ((*your_chrome_it)->display_name,
                  l10n_util::GetStringUTF8(
                      IDS_CUSTOMIZE_TOOLBAR_CATEGORY_BRAVE_MENU));
      }));
}

}  // namespace customize_chrome
