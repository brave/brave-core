// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/partitioned_storage_page_action_controller.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/scoped_observation.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/containers/containers_service_factory.h"
#include "brave/components/containers/content/browser/containers_web_contents_user_data.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "brave/components/containers/core/browser/containers_test_utils.h"
#include "brave/components/containers/core/browser/prefs.h"
#include "brave/components/containers/core/common/features.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/page_action/page_action_controller.h"
#include "chrome/browser/ui/page_action/page_action_icon_type.h"
#include "chrome/browser/ui/page_action/page_action_model.h"
#include "chrome/browser/ui/page_action/page_action_model_observer.h"
#include "chrome/browser/ui/page_action/test_support/fake_tab_interface.h"
#include "chrome/browser/ui/page_action/test_support/test_page_action_properties_provider.h"
#include "chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model.h"
#include "chrome/test/base/testing_profile.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/sync_preferences/pref_service_mock_factory.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/actions/actions.h"

namespace page_actions {

namespace {

// Records the most recent state pushed into the action's PageActionModel.
class TestObserver : public PageActionModelObserver {
 public:
  TestObserver() = default;
  ~TestObserver() override = default;

  // PageActionModelObserver:
  void OnPageActionModelChanged(
      const PageActionModelInterface& model) override {
    visible_ = model.GetVisible();
    text_ = model.GetText();
    tooltip_text_ = model.GetTooltipText();
    ++model_change_count_;
  }

  bool visible() const { return visible_; }
  const std::u16string& text() const { return text_; }
  const std::u16string& tooltip_text() const { return tooltip_text_; }
  int model_change_count() const { return model_change_count_; }

 private:
  bool visible_ = false;
  std::u16string text_;
  std::u16string tooltip_text_;
  int model_change_count_ = 0;
};

// ContainersService can't be built via its factory in this lightweight
// target (it depends on SessionServiceFactory/TabRestoreServiceFactory), so
// it's constructed directly with a mock delegate, mirroring
// ContainersServiceTest (containers_service_unittest.cc).
std::unique_ptr<KeyedService> BuildTestContainersService(
    content::BrowserContext* context) {
  Profile* profile = Profile::FromBrowserContext(context);
  return std::make_unique<containers::ContainersService>(
      profile->GetPrefs(), profile->IsOffTheRecord(),
      std::make_unique<containers::MockContainersServiceDelegate>());
}

}  // namespace

class PartitionedStoragePageActionControllerTest : public testing::Test {
 public:
  PartitionedStoragePageActionControllerTest()
      : feature_list_(containers::features::kContainers),
        properties_provider_(PageActionPropertiesMap{{
            kActionShowPartitionedStorage,
            PageActionProperties{
                .histogram_name = "PartitionedStorage",
                .type = brave::kPartitionedStorageActionIconType,
            },
        }}) {}

  void SetUp() override {
    auto registry = base::MakeRefCounted<user_prefs::PrefRegistrySyncable>();
    RegisterUserProfilePrefs(registry.get());

    sync_preferences::PrefServiceMockFactory factory;
    auto pref_service = factory.CreateSyncable(registry.get());

    TestingProfile::Builder builder;
    builder.SetPrefService(std::move(pref_service));
    builder.AddTestingFactory(ContainersServiceFactory::GetInstance(),
                              base::BindOnce(&BuildTestContainersService));
    profile_ = builder.Build();

    pinned_actions_model_ =
        std::make_unique<PinnedToolbarActionsModel>(profile_.get());
    tab_interface_ = std::make_unique<FakeTabInterface>(profile_.get());
    tab_interface_->Activate();

    page_action_controller_ = std::make_unique<PageActionControllerImpl>(
        *tab_interface_,
        std::vector<actions::ActionId>{kActionShowPartitionedStorage},
        properties_provider_, pinned_actions_model_.get());

    action_item_ = actions::ActionItem::Builder()
                       .SetActionId(kActionShowPartitionedStorage)
                       .SetVisible(true)
                       .SetEnabled(true)
                       .Build();
    action_item_subscription_ =
        page_action_controller_->CreateActionItemSubscription(
            action_item_.get());

    page_action_controller_->AddObserver(kActionShowPartitionedStorage,
                                         observation_);

    controller_ = std::make_unique<PartitionedStoragePageActionController>(
        *tab_interface_, *page_action_controller_);
    controller_->Init();
  }

  void TearDown() override {
    observation_.Reset();
    controller_.reset();
    action_item_subscription_ = base::CallbackListSubscription();
    action_item_.reset();
    page_action_controller_.reset();
    tab_interface_.reset();
    pinned_actions_model_.reset();
    profile_.reset();
  }

  FakeTabInterface* tab_interface() { return tab_interface_.get(); }
  TestingProfile* profile() { return profile_.get(); }
  PartitionedStoragePageActionController* controller() {
    return controller_.get();
  }
  const TestObserver& observer() const { return observer_; }

 private:
  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  TestPageActionPropertiesProvider properties_provider_;

  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<PinnedToolbarActionsModel> pinned_actions_model_;
  std::unique_ptr<FakeTabInterface> tab_interface_;
  std::unique_ptr<PageActionControllerImpl> page_action_controller_;
  std::unique_ptr<actions::ActionItem> action_item_;
  base::CallbackListSubscription action_item_subscription_;

  TestObserver observer_;
  base::ScopedObservation<PageActionModelInterface, PageActionModelObserver>
      observation_{&observer_};

  std::unique_ptr<PartitionedStoragePageActionController> controller_;
};

TEST_F(PartitionedStoragePageActionControllerTest, HiddenByDefault) {
  EXPECT_FALSE(observer().visible());
}

TEST_F(PartitionedStoragePageActionControllerTest,
       VisibleWhenTabHasContainerId) {
  ASSERT_FALSE(observer().visible());

  containers::ContainersWebContentsUserData::CreateForWebContents(
      tab_interface()->GetContents(), "test-container");
  tab_interface()->Activate();

  ASSERT_TRUE(observer().visible());
  // CreateUnknownContainer() (the fallback for an id with no synced
  // metadata) uses the first 8 characters of the id as the display name.
  EXPECT_EQ(observer().tooltip_text(), u"test-con");
}

TEST_F(PartitionedStoragePageActionControllerTest,
       TruncatesLongKnownContainerName) {
  const std::string long_name(30, 'a');
  std::vector<containers::mojom::ContainerPtr> synced_containers;
  synced_containers.push_back(
      containers::MakeContainer("known-container", long_name));
  containers::SetContainersToPrefs(synced_containers, *profile()->GetPrefs());

  containers::ContainersWebContentsUserData::CreateForWebContents(
      tab_interface()->GetContents(), "known-container");
  tab_interface()->Activate();

  ASSERT_TRUE(observer().visible());
  EXPECT_EQ(observer().tooltip_text(), base::UTF8ToUTF16(long_name));
  // OverrideText() is truncated to keep the PageActionView from growing
  // unbounded; OverrideTooltip() above is not.
  EXPECT_NE(observer().text(), observer().tooltip_text());
  EXPECT_LE(observer().text().size(), 20u);
}

TEST_F(PartitionedStoragePageActionControllerTest,
       ExecuteActionNoopWithoutToolbarButtonProvider) {
  controller()->ExecuteAction(/*toolbar_button_provider=*/nullptr,
                              /*item=*/nullptr);
}

}  // namespace page_actions
