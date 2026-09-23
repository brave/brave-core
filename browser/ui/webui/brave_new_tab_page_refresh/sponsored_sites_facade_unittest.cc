// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/sponsored_sites_facade.h"

#include <memory>
#include <string>
#include <vector>

#include "base/check.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page.mojom.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/prefs/pref_registry.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_sites_data.h"
#include "brave/components/ntp_background_images/browser/test/fake_ntp_background_images_service.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/ntp_background_images/common/view_counter_pref_registry.h"
#include "components/history/core/browser/history_database_params.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/test/history_service_test_util.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_rewards/core/pref_registry.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

namespace brave_new_tab_page_refresh {

namespace {

constexpr char kTestSponsoredSitesUrlPrefix[] =
    "chrome://branded-wallpaper/sponsored-images/";

struct SponsoredSiteTileSpec {
  std::string title;
  std::string target_url;
};

ntp_background_images::NTPSponsoredSitesData CreateSponsoredSitesData(
    const base::FilePath& installed_dir,
    const std::vector<SponsoredSiteTileSpec>& tiles) {
  std::vector<std::string> tile_json_strings;
  for (const auto& tile : tiles) {
    const base::FilePath image_dir = installed_dir.AppendASCII(tile.title);
    CHECK(base::CreateDirectory(image_dir));
    CHECK(base::WriteFile(image_dir.AppendASCII("logo.webp"), ""));

    tile_json_strings.push_back(absl::StrFormat(
        R"(
        {
          "version": 1,
          "title": "%s",
          "adDisclosure": "baz",
          "targetUrl": "%s",
          "image": {
            "relativeUrl": "%s/logo.webp"
          }
        })",
        tile.title, tile.target_url, tile.title));
  }

  const base::DictValue dict = base::test::ParseJsonDict(absl::StrFormat(
      R"(
      {
        "schemaVersion": 1,
        "tiles": [%s]
      })",
      base::JoinString(tile_json_strings, ",")));

  return ntp_background_images::NTPSponsoredSitesData(
      dict, installed_dir, kTestSponsoredSitesUrlPrefix);
}

ntp_background_images::NTPSponsoredSitesData CreateSponsoredSitesData(
    const base::FilePath& installed_dir,
    const std::string& title,
    const std::string& target_url) {
  return CreateSponsoredSitesData(installed_dir,
                                  {SponsoredSiteTileSpec{title, target_url}});
}

}  // namespace

class SponsoredSitesFacadeTest : public testing::Test {
 public:
  void SetUp() override {
    ntp_background_images::RegisterProfilePrefs(profile_prefs_.registry());
    brave_ads::RegisterProfilePrefs(profile_prefs_.registry());
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
    brave_rewards::RegisterProfilePrefs(profile_prefs_.registry());
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

    ntp_background_images::NTPBackgroundImagesService::
        RegisterLocalStatePrefsForMigration(local_state_.registry());
    background_images_service_ =
        std::make_unique<ntp_background_images::FakeNTPBackgroundImagesService>(
            /*variations_service=*/nullptr,
            /*component_update_service=*/nullptr, &local_state_);

    ASSERT_TRUE(installed_dir_.CreateUniqueTempDir());

    ASSERT_TRUE(history_dir_.CreateUniqueTempDir());
    history_service_ = history::CreateHistoryService(history_dir_.GetPath(),
                                                     /*create_db=*/true);
    ASSERT_TRUE(history_service_);
  }

  // Ensures the backend has no pending work on its own sequence before the
  // temp directory it reads from is torn down.
  void TearDown() override {
    history::BlockUntilHistoryProcessesPendingRequests(history_service_.get());
    history_service_.reset();
  }

  std::unique_ptr<SponsoredSitesFacade> CreateFacade() {
    return std::make_unique<SponsoredSitesFacade>(
        profile_prefs_, background_images_service_.get(), *history_service_);
  }

  std::vector<mojom::SponsoredSitePtr> GetSites(SponsoredSitesFacade& facade) {
    base::test::TestFuture<std::vector<mojom::SponsoredSitePtr>> future;
    facade.GetSites(future.GetCallback());
    return future.Take();
  }

  // Sets `target_url` as the sole sponsored site and waits for its History
  // lookup to resolve. The lookup is only kicked off by `GetSites()` itself,
  // so an initial call is needed before the tracker has anything pending.
  // Waits on the tracker's pending-task state rather than the sites-updated
  // callback, since that callback is skipped when nothing changed from the
  // fail-closed default.
  bool GetHasGenuineVisit(SponsoredSitesFacade& facade,
                          const std::string& target_url) {
    background_images_service_->OnGetSponsoredSitesData(
        CreateSponsoredSitesData(installed_dir_.GetPath(), "foo", target_url));
    GetSites(facade);
    EXPECT_TRUE(base::test::RunUntil(
        [&] { return !facade.HasPendingGenuineVisitQueriesForTesting(); }));

    auto sites = GetSites(facade);
    CHECK_EQ(sites.size(), 1u);
    return sites[0]->has_genuine_visit;
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
  TestingPrefServiceSimple local_state_;
  base::ScopedTempDir installed_dir_;
  std::unique_ptr<ntp_background_images::FakeNTPBackgroundImagesService>
      background_images_service_;
  base::ScopedTempDir history_dir_;
  std::unique_ptr<history::HistoryService> history_service_;
};

TEST_F(SponsoredSitesFacadeTest, ReturnsNoSitesWhenSponsoredSitesDisabled) {
  profile_prefs_.SetBoolean(brave_ads::prefs::kSponsoredEnabled, false);

  auto facade = CreateFacade();
  background_images_service_->OnGetSponsoredSitesData(CreateSponsoredSitesData(
      installed_dir_.GetPath(), "foo", "https://foo.com"));

  EXPECT_THAT(GetSites(*facade), testing::IsEmpty());
}

TEST_F(SponsoredSitesFacadeTest, ReturnsNoSitesWhenNoSponsoredSitesData) {
  auto facade = CreateFacade();

  EXPECT_THAT(GetSites(*facade), testing::IsEmpty());
}

TEST_F(SponsoredSitesFacadeTest, ReturnsEligibleSites) {
  auto facade = CreateFacade();
  background_images_service_->OnGetSponsoredSitesData(CreateSponsoredSitesData(
      installed_dir_.GetPath(),
      {SponsoredSiteTileSpec{"foo", "https://foo.com"},
       SponsoredSiteTileSpec{"bar", "https://bar.com"}}));

  EXPECT_THAT(GetSites(*facade),
              testing::ElementsAre(testing::Pointee(testing::Field(
                                       &mojom::SponsoredSite::title, "foo")),
                                   testing::Pointee(testing::Field(
                                       &mojom::SponsoredSite::title, "bar"))));
}

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
TEST_F(SponsoredSitesFacadeTest, ReturnsNoSitesWhenRewardsWalletConnected) {
  profile_prefs_.SetString(brave_rewards::prefs::kExternalWalletType, "uphold");

  auto facade = CreateFacade();
  background_images_service_->OnGetSponsoredSitesData(CreateSponsoredSitesData(
      installed_dir_.GetPath(), "foo", "https://foo.com"));

  EXPECT_THAT(GetSites(*facade), testing::IsEmpty());
}
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

TEST_F(SponsoredSitesFacadeTest, InvokesCallbackWhenSponsoredSitesDataUpdates) {
  auto facade = CreateFacade();

  bool callback_invoked = false;
  facade->SetSitesUpdatedCallback(base::BindLambdaForTesting(
      [&callback_invoked] { callback_invoked = true; }));

  background_images_service_->OnGetSponsoredSitesData(CreateSponsoredSitesData(
      installed_dir_.GetPath(), "foo", "https://foo.com"));

  EXPECT_TRUE(callback_invoked);
}

TEST_F(SponsoredSitesFacadeTest, NoGenuineVisitWhenAdvertiserHasNoHistory) {
  auto facade = CreateFacade();

  EXPECT_FALSE(GetHasGenuineVisit(*facade, "https://foo.com"));
}

TEST_F(SponsoredSitesFacadeTest, GenuineVisitWhenAdvertiserWasVisited) {
  auto facade = CreateFacade();
  history_service_->AddPage(GURL("https://foo.com/"), base::Time::Now(),
                            history::VisitSource::SOURCE_BROWSED);

  EXPECT_TRUE(GetHasGenuineVisit(*facade, "https://foo.com"));
}

TEST_F(SponsoredSitesFacadeTest, GenuineVisitMatchesByHostNotExactUrl) {
  auto facade = CreateFacade();
  history_service_->AddPage(GURL("https://foo.com/some/page?q=1"),
                            base::Time::Now(),
                            history::VisitSource::SOURCE_BROWSED);

  EXPECT_TRUE(GetHasGenuineVisit(*facade, "https://foo.com"));
}

TEST_F(SponsoredSitesFacadeTest, GenuineVisitMatchesWwwVariant) {
  auto facade = CreateFacade();
  history_service_->AddPage(GURL("https://www.foo.com/"), base::Time::Now(),
                            history::VisitSource::SOURCE_BROWSED);

  EXPECT_TRUE(GetHasGenuineVisit(*facade, "https://foo.com"));
}

TEST_F(SponsoredSitesFacadeTest, PreservesGenuineVisitAcrossRoutineRefresh) {
  auto facade = CreateFacade();
  history_service_->AddPage(GURL("https://foo.com/"), base::Time::Now(),
                            history::VisitSource::SOURCE_BROWSED);
  ASSERT_TRUE(GetHasGenuineVisit(*facade, "https://foo.com"));

  // A routine refresh re-fetches the same sponsored sites data.
  background_images_service_->OnGetSponsoredSitesData(CreateSponsoredSitesData(
      installed_dir_.GetPath(), "foo", "https://foo.com"));

  auto sites = GetSites(*facade);
  ASSERT_EQ(sites.size(), 1u);
  EXPECT_TRUE(sites[0]->has_genuine_visit);
}

}  // namespace brave_new_tab_page_refresh
