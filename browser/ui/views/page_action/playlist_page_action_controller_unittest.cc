// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/playlist_page_action_controller.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/scoped_observation.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/browser/ui/views/page_action/page_action_test_observer.h"
#include "brave/browser/ui/views/page_action/test_tab_interface.h"
#include "brave/components/playlist/content/browser/media_detector_component_manager.h"
#include "brave/components/playlist/content/browser/playlist_service.h"
#include "brave/components/playlist/content/browser/playlist_tab_helper.h"
#include "brave/components/playlist/core/common/features.h"
#include "brave/components/playlist/core/common/mojom/playlist.mojom.h"
#include "brave/components/playlist/core/common/pref_names.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/page_action/page_action_controller.h"
#include "chrome/browser/ui/page_action/page_action_icon_type.h"
#include "chrome/browser/ui/page_action/page_action_model.h"
#include "chrome/browser/ui/page_action/page_action_model_observer.h"
#include "chrome/browser/ui/page_action/test_support/test_page_action_properties_provider.h"
#include "chrome/browser/ui/toolbar/pinned_toolbar/pinned_toolbar_actions_model.h"
#include "chrome/test/base/testing_profile.h"
#include "components/download/public/common/download_task_runner.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/pref_service_mock_factory.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/actions/actions.h"

namespace page_actions {

namespace {

void AttachTabHelpers(playlist::PlaylistService* service,
                      content::WebContents* contents) {
  playlist::PlaylistTabHelper::CreateForWebContents(contents, service);
}

}  // namespace

class PlaylistPageActionControllerTest : public testing::Test {
 public:
  PlaylistPageActionControllerTest()
      : feature_list_(playlist::features::kPlaylist),
        properties_provider_(PageActionPropertiesMap{{
            kActionShowPlaylistPageAction,
            PageActionProperties{
                .histogram_name = "Playlist",
                .type = brave::kPlaylistPageActionIconType,
            },
        }}) {}

  // testing::Test:
  void SetUp() override {
    // Mirrors PlaylistServiceUnitTest's setup (browser/playlist/test/
    // playlist_service_unittest.cc): PlaylistService can't be built via its
    // factory in this lightweight target, so it's constructed directly.
    auto registry = base::MakeRefCounted<user_prefs::PrefRegistrySyncable>();
    playlist::PlaylistServiceFactory::GetInstance();
    RegisterUserProfilePrefs(registry.get());
    playlist::PlaylistServiceFactory::RegisterLocalStatePrefs(
        local_state_.registry());

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    sync_preferences::PrefServiceMockFactory factory;
    auto pref_service = factory.CreateSyncable(registry.get());

    TestingProfile::Builder builder;
    builder.SetPrefService(std::move(pref_service));
    builder.SetPath(temp_dir_.GetPath());
    profile_ = builder.Build();
    profile_->GetPrefs()->SetBoolean(playlist::kPlaylistEnabledPref, true);

    DCHECK(!download::GetIOTaskRunner());
    download::SetIOTaskRunner(
        base::SingleThreadTaskRunner::GetCurrentDefault());

    detector_manager_ =
        std::make_unique<playlist::MediaDetectorComponentManager>(nullptr);
    detector_manager_->SetUseLocalScript();
    playlist_service_ = std::make_unique<playlist::PlaylistService>(
        profile_.get(), &local_state_, detector_manager_.get(), nullptr,
        base::Time::Now());

    pinned_actions_model_ =
        std::make_unique<PinnedToolbarActionsModel>(profile_.get());
    tab_interface_ = std::make_unique<TestTabInterface>(
        profile_.get(),
        base::BindRepeating(&AttachTabHelpers, playlist_service_.get()));
    tab_interface_->Activate();

    page_action_controller_ = std::make_unique<PageActionControllerImpl>(
        *tab_interface_,
        std::vector<actions::ActionId>{kActionShowPlaylistPageAction},
        properties_provider_, pinned_actions_model_.get());

    action_item_ = actions::ActionItem::Builder()
                       .SetActionId(kActionShowPlaylistPageAction)
                       .SetVisible(true)
                       .SetEnabled(true)
                       .Build();
    action_item_subscription_ =
        page_action_controller_->CreateActionItemSubscription(
            action_item_.get());

    page_action_controller_->AddObserver(kActionShowPlaylistPageAction,
                                         observation_);

    controller_ = std::make_unique<PlaylistPageActionController>(
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
    playlist_service_.reset();
    detector_manager_.reset();
    profile_.reset();

    download::ClearIOTaskRunnerForTesting();
  }

  playlist::PlaylistTabHelper* tab_helper() {
    return playlist::PlaylistTabHelper::FromWebContents(
        tab_interface_->GetContents());
  }

  // Mirrors PlaylistTabHelper::OnMediaFilesUpdated(), a public
  // mojom::PlaylistServiceObserver override that the real PlaylistService
  // calls when it detects media on the current page.
  void AddFoundItem() { AddFoundItem(tab_helper()); }
  void AddFoundItem(playlist::PlaylistTabHelper* tab_helper) {
    std::vector<playlist::mojom::PlaylistItemPtr> items;
    items.push_back(playlist::mojom::PlaylistItem::New());
    tab_helper->OnMediaFilesUpdated(
        tab_interface_->GetContents()->GetLastCommittedURL(), std::move(items));
  }

  // Mirrors PlaylistTabHelper::OnItemCreated(), a public
  // mojom::PlaylistServiceObserver override that the real PlaylistService
  // calls once an item has actually been saved.
  void AddSavedItem() {
    auto item = playlist::mojom::PlaylistItem::New();
    item->id = "test-item";
    item->page_source = tab_interface_->GetContents()->GetLastCommittedURL();
    tab_helper()->OnItemCreated(std::move(item));
  }

  TestTabInterface& tab_interface() { return *tab_interface_; }

  const PageActionTestObserver& observer() const { return observer_; }
  PrefService* prefs() { return profile_->GetPrefs(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  TestPageActionPropertiesProvider properties_provider_;

  TestingPrefServiceSimple local_state_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<playlist::MediaDetectorComponentManager> detector_manager_;
  std::unique_ptr<playlist::PlaylistService> playlist_service_;

  std::unique_ptr<PinnedToolbarActionsModel> pinned_actions_model_;
  std::unique_ptr<TestTabInterface> tab_interface_;
  std::unique_ptr<PageActionControllerImpl> page_action_controller_;
  std::unique_ptr<actions::ActionItem> action_item_;
  base::CallbackListSubscription action_item_subscription_;

  PageActionTestObserver observer_;
  base::ScopedObservation<PageActionModelInterface, PageActionModelObserver>
      observation_{&observer_};

  std::unique_ptr<PlaylistPageActionController> controller_;
};

TEST_F(PlaylistPageActionControllerTest, HiddenByDefault) {
  EXPECT_FALSE(observer().visible());
}

TEST_F(PlaylistPageActionControllerTest, VisibleWithFoundItem) {
  AddFoundItem();

  EXPECT_TRUE(observer().visible());
}

TEST_F(PlaylistPageActionControllerTest, VisibleWithSavedItem) {
  AddSavedItem();

  EXPECT_TRUE(observer().visible());
}

// The tab's contents can be swapped out - by tab discarding, or by a shared
// pinned tab being moved to another window. The controller has to follow the
// swapped in contents, and stop observing the outgoing contents' tab helper,
// which is about to be destroyed.
TEST_F(PlaylistPageActionControllerTest, FollowsDiscardedContents) {
  playlist::PlaylistTabHelper* const discarded_tab_helper = tab_helper();
  AddFoundItem(discarded_tab_helper);
  ASSERT_TRUE(observer().visible());

  // The swapped in contents has no items, so the action hides.
  tab_interface().DiscardContents();
  ASSERT_NE(discarded_tab_helper, tab_helper());
  EXPECT_FALSE(observer().visible());

  // The discarded contents no longer drives the page action.
  const int model_change_count = observer().model_change_count();
  AddFoundItem(discarded_tab_helper);
  EXPECT_EQ(model_change_count, observer().model_change_count());

  // The swapped in one does.
  AddFoundItem();
  EXPECT_TRUE(observer().visible());
}

TEST_F(PlaylistPageActionControllerTest, HiddenWhenPrefDisabled) {
  prefs()->SetBoolean(playlist::kPlaylistEnabledPref, false);
  AddSavedItem();

  EXPECT_FALSE(observer().visible());
}

}  // namespace page_actions
