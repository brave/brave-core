// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_ads/ads_tab_helper.h"

#include <memory>

#include "base/check.h"
#include "base/memory/raw_ptr.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/browser/service/test/ads_service_mock.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/profile/test/test_profile_ios.h"
#include "ios/web/public/test/fakes/fake_navigation_context.h"
#include "ios/web/public/test/fakes/fake_navigation_manager.h"
#include "ios/web/public/test/fakes/fake_web_frame.h"
#include "ios/web/public/test/fakes/fake_web_frames_manager.h"
#include "ios/web/public/test/fakes/fake_web_state.h"
#include "ios/web/public/test/web_task_environment.h"
#include "ios/web/public/web_state_observer.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

constexpr char kInnerText[] = "Inner text";

class NavigationBuilder final {
 public:
  NavigationBuilder(web::FakeWebState& web_state, const GURL& url)
      : web_state_(web_state) {
    navigation_context_.SetUrl(url);
  }

  NavigationBuilder(const NavigationBuilder&) = delete;
  NavigationBuilder& operator=(const NavigationBuilder&) = delete;

  ~NavigationBuilder() = default;

  NavigationBuilder& WithPageTransition(ui::PageTransition page_transition) {
    navigation_context_.SetPageTransition(page_transition);
    return *this;
  }

  NavigationBuilder& WithIsSameDocument(bool is_same_document) {
    navigation_context_.SetIsSameDocument(is_same_document);
    return *this;
  }

  NavigationBuilder& WithResponseHeaders(
      scoped_refptr<net::HttpResponseHeaders> headers) {
    navigation_context_.SetResponseHeaders(std::move(headers));
    return *this;
  }

  NavigationBuilder& WithError(NSError* error) {
    navigation_context_.SetError(error);
    return *this;
  }

  void Simulate() {
    navigation_context_.SetHasCommitted(true);
    web_state_->OnNavigationStarted(&navigation_context_);
    web_state_->OnNavigationFinished(&navigation_context_);
  }

 private:
  raw_ref<web::FakeWebState> web_state_;
  web::FakeNavigationContext navigation_context_;
};

// Adds support for simulating a native session restore, which
// `web::FakeNavigationManager` does not expose a setter for.
class FakeNavigationManagerWithRestore final
    : public web::FakeNavigationManager {
 public:
  bool IsNativeRestoreInProgress() const override { return is_restoring_; }

  void SetNativeRestoreInProgress(bool is_restoring) {
    is_restoring_ = is_restoring;
  }

 private:
  bool is_restoring_ = false;
};

}  // namespace

class AdsTabHelperTest : public PlatformTest {
 public:
  AdsTabHelperTest() {
    profile_ = TestProfileIOS::Builder().Build();

    profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);
    profile_->GetPrefs()->SetBoolean(prefs::kNotificationsEnabled, true);

    web_state_ = std::make_unique<web::FakeWebState>();
    web_state_->SetBrowserState(profile_.get());

    auto web_frames_manager = std::make_unique<web::FakeWebFramesManager>();
    web_frames_manager_ = web_frames_manager.get();
    web_state_->SetWebFramesManager(web::ContentWorld::kIsolatedWorld,
                                    std::move(web_frames_manager));

    auto main_web_frame = web::FakeWebFrame::CreateMainWebFrame();
    main_web_frame_ = main_web_frame.get();
    web_frames_manager_->AddWebFrame(std::move(main_web_frame));

    auto navigation_manager =
        std::make_unique<FakeNavigationManagerWithRestore>();
    navigation_manager_ = navigation_manager.get();
    web_state_->SetNavigationManager(std::move(navigation_manager));

    AdsTabHelper::CreateForWebState(web_state_.get(), &ads_service_mock_);
  }

  AdsTabHelperTest(const AdsTabHelperTest&) = delete;
  AdsTabHelperTest& operator=(const AdsTabHelperTest&) = delete;

  ~AdsTabHelperTest() override = default;

  void DisableBraveRewards() {
    profile_->GetPrefs()->SetBoolean(brave_rewards::prefs::kEnabled, false);
  }

  void DisableNotificationAds() {
    profile_->GetPrefs()->SetBoolean(prefs::kNotificationsEnabled, false);
  }

  NavigationBuilder Navigation(const GURL& url) {
    return NavigationBuilder(*web_state_, url);
  }

  // Waits for the frame's already-posted callback to run first.
  void SimulatePageLoad(const std::string& inner_text) {
    page_load_js_result_ = base::Value(inner_text);
    main_web_frame_->AddResultForExecutedJs(&page_load_js_result_,
                                            u"document?.body?.innerText");
    web_state_->OnPageLoaded(web::PageLoadCompletionStatus::SUCCESS);

    base::test::TestFuture<void> test_future;
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, test_future.GetCallback());
    ASSERT_TRUE(test_future.Wait());
  }

  void SimulateVisibilityChanged(bool is_visible) {
    if (is_visible) {
      web_state_->WasShown();
    } else {
      web_state_->WasHidden();
    }
  }

  void SimulateCloseTab() { web_state_.reset(); }

  AdsServiceMock& ads_service_mock() { return ads_service_mock_; }

  AdsTabHelper* ads_tab_helper() {
    return AdsTabHelper::FromWebState(web_state_.get());
  }

  FakeNavigationManagerWithRestore& navigation_manager() {
    CHECK(navigation_manager_);
    return *navigation_manager_;
  }

 private:
  web::WebTaskEnvironment task_environment_;
  std::unique_ptr<TestProfileIOS> profile_;
  // Declared before `web_state_` so it outlives it: destroying `web_state_`,
  // whether via `SimulateCloseTab` or fixture teardown, notifies
  // `AdsTabHelper`, which calls into this mock.
  AdsServiceMock ads_service_mock_;
  std::unique_ptr<web::FakeWebState> web_state_;
  raw_ptr<web::FakeWebFramesManager> web_frames_manager_;
  raw_ptr<web::FakeWebFrame> main_web_frame_;
  raw_ptr<FakeNavigationManagerWithRestore> navigation_manager_;
  // The frame keeps a pointer to this, so it must outlive `SimulatePageLoad`.
  base::Value page_load_js_result_;
};

TEST_F(AdsTabHelperTest, NotifyTabDidChange) {
  EXPECT_CALL(ads_service_mock(),
              NotifyTabDidChange(/*tab_id=*/testing::_,
                                 /*redirect_chain=*/testing::_,
                                 /*is_new_navigation=*/true,
                                 /*is_restoring=*/false,
                                 /*is_visible=*/testing::_));
  Navigation(GURL("https://brave.com")).Simulate();
}

TEST_F(AdsTabHelperTest, NotifyTabDidChangeIfTabWasRestored) {
  navigation_manager().SetNativeRestoreInProgress(true);
  EXPECT_CALL(ads_service_mock(),
              NotifyTabDidChange(/*tab_id=*/testing::_,
                                 /*redirect_chain=*/testing::_,
                                 /*is_new_navigation=*/testing::_,
                                 /*is_restoring=*/true,
                                 /*is_visible=*/testing::_));

  Navigation(GURL("https://brave.com")).Simulate();
}

TEST_F(
    AdsTabHelperTest,
    NotifyTabDidChangeIfTabWasRestoredForSameDocumentNavigationBeforePageLoaded) {
  navigation_manager().SetNativeRestoreInProgress(true);
  Navigation(GURL("https://brave.com")).Simulate();
  Navigation(GURL("https://brave.com#anchor"))
      .WithIsSameDocument(true)
      .Simulate();
  EXPECT_CALL(ads_service_mock(),
              NotifyTabDidChange(/*tab_id=*/testing::_,
                                 /*redirect_chain=*/testing::_,
                                 /*is_new_navigation=*/testing::_,
                                 /*is_restoring=*/true,
                                 /*is_visible=*/testing::_));

  SimulateVisibilityChanged(true);
}

TEST_F(
    AdsTabHelperTest,
    NotifyTabDidChangeIfTabWasNotRestoredForSameDocumentNavigationAfterPageLoaded) {
  navigation_manager().SetNativeRestoreInProgress(true);
  Navigation(GURL("https://brave.com")).Simulate();
  SimulatePageLoad(kInnerText);
  Navigation(GURL("https://brave.com#anchor"))
      .WithIsSameDocument(true)
      .Simulate();
  EXPECT_CALL(ads_service_mock(),
              NotifyTabDidChange(/*tab_id=*/testing::_,
                                 /*redirect_chain=*/testing::_,
                                 /*is_new_navigation=*/testing::_,
                                 /*is_restoring=*/false,
                                 /*is_visible=*/testing::_));

  SimulateVisibilityChanged(true);
}

TEST_F(
    AdsTabHelperTest,
    NotifyTabDidChangeIfTabWasRestoredForSameDocumentNavigationAfterErrorPage) {
  Navigation(GURL("https://brave.com")).Simulate();
  SimulatePageLoad(kInnerText);
  navigation_manager().SetNativeRestoreInProgress(true);
  NSError* error = [NSError errorWithDomain:@"test" code:-1 userInfo:nil];
  Navigation(GURL("https://brave.com/error")).WithError(error).Simulate();
  Navigation(GURL("https://brave.com/error#anchor"))
      .WithIsSameDocument(true)
      .Simulate();
  EXPECT_CALL(ads_service_mock(),
              NotifyTabDidChange(/*tab_id=*/testing::_,
                                 /*redirect_chain=*/testing::_,
                                 /*is_new_navigation=*/testing::_,
                                 /*is_restoring=*/true,
                                 /*is_visible=*/testing::_));

  SimulateVisibilityChanged(true);
}

TEST_F(AdsTabHelperTest, NotifyTabDidLoadForHttpSuccessfulResponsePage) {
  EXPECT_CALL(ads_service_mock(),
              NotifyTabDidLoad(/*tab_id=*/testing::_, net::HTTP_OK));
  Navigation(GURL("https://brave.com")).Simulate();
}

TEST_F(AdsTabHelperTest, NotifyTabDidLoadForHttpClientErrorResponsePage) {
  EXPECT_CALL(ads_service_mock(),
              NotifyTabDidLoad(/*tab_id=*/testing::_, net::HTTP_NOT_FOUND));
  Navigation(GURL("https://brave.com"))
      .WithResponseHeaders(net::HttpResponseHeaders::TryToCreate(
          "HTTP/1.1 404 Not Found\r\n\r\n"))
      .Simulate();
}

TEST_F(AdsTabHelperTest, NotifyTabDidLoadForHttpServerErrorResponsePage) {
  EXPECT_CALL(
      ads_service_mock(),
      NotifyTabDidLoad(/*tab_id=*/testing::_, net::HTTP_INTERNAL_SERVER_ERROR));
  Navigation(GURL("https://brave.com"))
      .WithResponseHeaders(net::HttpResponseHeaders::TryToCreate(
          "HTTP/1.1 500 Internal Server Error\r\n\r\n"))
      .Simulate();
}

TEST_F(AdsTabHelperTest, DoNotNotifyTabDidLoadForNetErrorPage) {
  NSError* error = [NSError errorWithDomain:@"test" code:-1 userInfo:nil];
  EXPECT_CALL(ads_service_mock(), NotifyTabDidLoad).Times(0);
  EXPECT_CALL(ads_service_mock(), NotifyTabDidFailToLoad);

  Navigation(GURL("https://brave.com")).WithError(error).Simulate();
}

TEST_F(AdsTabHelperTest,
       NotifyTabTextContentDidChangeForRewardsUserWithNotificationAdsEnabled) {
  Navigation(GURL("https://brave.com")).Simulate();
  EXPECT_CALL(ads_service_mock(),
              NotifyTabTextContentDidChange(/*tab_id=*/testing::_,
                                            /*redirect_chain=*/testing::_,
                                            /*text=*/kInnerText));
  SimulatePageLoad(kInnerText);
}

TEST_F(AdsTabHelperTest, DoNotNotifyTabTextContentDidChangeForNonRewardsUser) {
  DisableBraveRewards();
  Navigation(GURL("https://brave.com")).Simulate();
  EXPECT_CALL(ads_service_mock(), NotifyTabTextContentDidChange).Times(0);
  SimulatePageLoad(kInnerText);
}

TEST_F(
    AdsTabHelperTest,
    DoNotNotifyTabTextContentDidChangeForNonRewardsUserWithNotificationAdsDisabled) {
  DisableBraveRewards();
  DisableNotificationAds();
  Navigation(GURL("https://brave.com")).Simulate();
  EXPECT_CALL(ads_service_mock(), NotifyTabTextContentDidChange).Times(0);
  SimulatePageLoad(kInnerText);
}

TEST_F(
    AdsTabHelperTest,
    DoNotNotifyTabTextContentDidChangeForRewardsUserWithNotificationAdsDisabled) {
  DisableNotificationAds();
  Navigation(GURL("https://brave.com")).Simulate();
  EXPECT_CALL(ads_service_mock(), NotifyTabTextContentDidChange).Times(0);
  SimulatePageLoad(kInnerText);
}

TEST_F(AdsTabHelperTest, DoNotNotifyTabTextContentDidChangeIfTabWasRestored) {
  navigation_manager().SetNativeRestoreInProgress(true);
  Navigation(GURL("https://brave.com")).Simulate();
  EXPECT_CALL(ads_service_mock(), NotifyTabTextContentDidChange).Times(0);
  SimulatePageLoad(kInnerText);
}

TEST_F(AdsTabHelperTest,
       DoNotNotifyTabTextContentDidChangeForSameDocumentNavigation) {
  Navigation(GURL("https://brave.com")).Simulate();
  SimulatePageLoad(kInnerText);
  EXPECT_CALL(ads_service_mock(), NotifyTabTextContentDidChange).Times(0);

  Navigation(GURL("https://brave.com#anchor"))
      .WithIsSameDocument(true)
      .Simulate();
  SimulatePageLoad(kInnerText);
}

TEST_F(AdsTabHelperTest,
       DoNotNotifyTabTextContentDidChangeForPreviouslyCommittedNavigation) {
  Navigation(GURL("https://brave.com")).Simulate();
  SimulatePageLoad(kInnerText);
  EXPECT_CALL(ads_service_mock(), NotifyTabTextContentDidChange).Times(0);

  Navigation(GURL("https://brave.com"))
      .WithPageTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();
  SimulatePageLoad(kInnerText);

  Navigation(GURL("https://brave.com"))
      .WithPageTransition(ui::PAGE_TRANSITION_FORWARD_BACK)
      .Simulate();
  SimulatePageLoad(kInnerText);

  Navigation(GURL("https://brave.com"))
      .WithPageTransition(ui::PAGE_TRANSITION_RELOAD)
      .Simulate();
  SimulatePageLoad(kInnerText);
}

TEST_F(AdsTabHelperTest,
       DoNotNotifyTabTextContentDidChangeForHttpClientErrorResponsePage) {
  EXPECT_CALL(ads_service_mock(), NotifyTabTextContentDidChange).Times(0);
  Navigation(GURL("https://brave.com"))
      .WithResponseHeaders(net::HttpResponseHeaders::TryToCreate(
          "HTTP/1.1 404 Not Found\r\n\r\n"))
      .Simulate();
  SimulatePageLoad(kInnerText);
}

TEST_F(AdsTabHelperTest,
       DoNotNotifyTabTextContentDidChangeForHttpServerErrorResponsePage) {
  EXPECT_CALL(ads_service_mock(), NotifyTabTextContentDidChange).Times(0);
  Navigation(GURL("https://brave.com"))
      .WithResponseHeaders(net::HttpResponseHeaders::TryToCreate(
          "HTTP/1.1 500 Internal Server Error\r\n\r\n"))
      .Simulate();
  SimulatePageLoad(kInnerText);
}

TEST_F(AdsTabHelperTest, NotifyTabDidStartPlayingMedia) {
  EXPECT_CALL(ads_service_mock(), NotifyTabDidStartPlayingMedia);

  ads_tab_helper()->NotifyTabDidStartPlayingMedia(/*player_id=*/1);
}

TEST_F(AdsTabHelperTest,
       DoNotNotifyTabDidStartPlayingMediaForAlreadyPlayingPlayer) {
  ads_tab_helper()->NotifyTabDidStartPlayingMedia(/*player_id=*/1);
  EXPECT_CALL(ads_service_mock(), NotifyTabDidStartPlayingMedia).Times(0);

  ads_tab_helper()->NotifyTabDidStartPlayingMedia(/*player_id=*/1);
}

TEST_F(AdsTabHelperTest,
       DoNotNotifyTabDidStartPlayingMediaWhenAnotherPlayerIsAlreadyPlaying) {
  ads_tab_helper()->NotifyTabDidStartPlayingMedia(/*player_id=*/1);
  EXPECT_CALL(ads_service_mock(), NotifyTabDidStartPlayingMedia).Times(0);

  ads_tab_helper()->NotifyTabDidStartPlayingMedia(/*player_id=*/2);
}

TEST_F(AdsTabHelperTest, NotifyTabDidStopPlayingMedia) {
  ads_tab_helper()->NotifyTabDidStartPlayingMedia(/*player_id=*/1);
  EXPECT_CALL(ads_service_mock(), NotifyTabDidStopPlayingMedia);

  ads_tab_helper()->NotifyTabDidStopPlayingMedia(/*player_id=*/1);
}

TEST_F(AdsTabHelperTest,
       DoNotNotifyTabDidStopPlayingMediaWhenAnotherPlayerIsStillPlaying) {
  ads_tab_helper()->NotifyTabDidStartPlayingMedia(/*player_id=*/1);
  ads_tab_helper()->NotifyTabDidStartPlayingMedia(/*player_id=*/2);
  EXPECT_CALL(ads_service_mock(), NotifyTabDidStopPlayingMedia).Times(0);

  ads_tab_helper()->NotifyTabDidStopPlayingMedia(/*player_id=*/1);
}

TEST_F(AdsTabHelperTest,
       DoNotNotifyTabDidStopPlayingMediaForPlayerThatWasNotPlaying) {
  EXPECT_CALL(ads_service_mock(), NotifyTabDidStopPlayingMedia).Times(0);

  ads_tab_helper()->NotifyTabDidStopPlayingMedia(/*player_id=*/1);
}

TEST_F(AdsTabHelperTest,
       NotifyTabDidStartPlayingMediaForReusedPlayerIdAfterNavigation) {
  ads_tab_helper()->NotifyTabDidStartPlayingMedia(/*player_id=*/1);
  Navigation(GURL("https://brave.com")).Simulate();
  EXPECT_CALL(ads_service_mock(), NotifyTabDidStartPlayingMedia);

  ads_tab_helper()->NotifyTabDidStartPlayingMedia(/*player_id=*/1);
}

TEST_F(AdsTabHelperTest, NotifyDidCloseTab) {
  EXPECT_CALL(ads_service_mock(), NotifyDidCloseTab);
  SimulateCloseTab();
}

}  // namespace brave_ads
