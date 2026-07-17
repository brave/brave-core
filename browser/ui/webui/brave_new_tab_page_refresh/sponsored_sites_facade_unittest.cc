// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/sponsored_sites_facade.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/check.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page.mojom.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_sites_data.h"
#include "brave/components/ntp_background_images/browser/test/fake_ntp_background_images_service.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/ntp_background_images/common/view_counter_pref_registry.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/test/history_service_test_util.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
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

ntp_background_images::NTPSponsoredSitesData CreateSponsoredSitesData(
    const base::FilePath& installed_dir,
    const std::string& title,
    const std::string& target_url) {
  const base::FilePath image_dir = installed_dir.AppendASCII(title);
  CHECK(base::CreateDirectory(image_dir));
  CHECK(base::WriteFile(image_dir.AppendASCII("logo.png"), ""));

  const base::DictValue dict = base::test::ParseJsonDict(absl::StrFormat(
      R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "%s",
            "adDisclosure": "Ad",
            "targetUrl": "%s",
            "image": {
              "relativeUrl": "%s/logo.png"
            }
          }
        ]
      })",
      title, target_url, title));

  return ntp_background_images::NTPSponsoredSitesData(
      dict, installed_dir, kTestSponsoredSitesUrlPrefix);
}

}  // namespace

class SponsoredSitesFacadeTest : public testing::Test {
 public:
  void SetUp() override {
    ntp_background_images::RegisterProfilePrefs(profile_prefs_.registry());
    profile_prefs_.registry()->RegisterBooleanPref(
        kNewTabPageShowSponsoredSites, true);
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
  }

  std::unique_ptr<history::HistoryService> CreateHistoryServiceWithVisit(
      const GURL& visited_url) {
    std::unique_ptr<history::HistoryService> history_service =
        history::CreateHistoryService(history_dir_.GetPath(),
                                      /*create_db=*/true);
    CHECK(history_service);
    if (visited_url.is_valid()) {
      history_service->AddPage(visited_url, base::Time::Now(),
                               history::SOURCE_BROWSED);
      history::BlockUntilHistoryProcessesPendingRequests(history_service.get());
    }
    return history_service;
  }

  std::vector<mojom::SponsoredSitePtr> GetSites(SponsoredSitesFacade& facade) {
    base::test::TestFuture<std::vector<mojom::SponsoredSitePtr>> future;
    facade.GetSites(future.GetCallback());
    return future.Take();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
  TestingPrefServiceSimple local_state_;
  base::ScopedTempDir installed_dir_;
  base::ScopedTempDir history_dir_;
  std::unique_ptr<ntp_background_images::FakeNTPBackgroundImagesService>
      background_images_service_;
};

TEST_F(SponsoredSitesFacadeTest, ReturnsNoSitesWhenSponsoredSitesDisabled) {
  profile_prefs_.SetBoolean(kNewTabPageShowSponsoredSites, false);

  std::unique_ptr<history::HistoryService> history_service =
      CreateHistoryServiceWithVisit(GURL());

  SponsoredSitesFacade facade(profile_prefs_, background_images_service_.get(),
                              history_service.get());
  background_images_service_->OnGetSponsoredSitesData(CreateSponsoredSitesData(
      installed_dir_.GetPath(), "foo", "https://foo.com"));

  EXPECT_TRUE(GetSites(facade).empty());
}

TEST_F(SponsoredSitesFacadeTest, ReturnsNoSitesWhenNoSponsoredSitesData) {
  std::unique_ptr<history::HistoryService> history_service =
      CreateHistoryServiceWithVisit(GURL());

  SponsoredSitesFacade facade(profile_prefs_, background_images_service_.get(),
                              history_service.get());

  EXPECT_TRUE(GetSites(facade).empty());
}

TEST_F(SponsoredSitesFacadeTest, ReturnsNoSitesWhenNoHistoryService) {
  SponsoredSitesFacade facade(profile_prefs_, background_images_service_.get(),
                              /*history_service=*/nullptr);
  background_images_service_->OnGetSponsoredSitesData(CreateSponsoredSitesData(
      installed_dir_.GetPath(), "foo", "https://foo.com"));

  EXPECT_TRUE(GetSites(facade).empty());
}

TEST_F(SponsoredSitesFacadeTest, FiltersOutSitesNeverVisited) {
  std::unique_ptr<history::HistoryService> history_service =
      CreateHistoryServiceWithVisit(GURL("https://unrelated.com"));

  SponsoredSitesFacade facade(profile_prefs_, background_images_service_.get(),
                              history_service.get());
  background_images_service_->OnGetSponsoredSitesData(CreateSponsoredSitesData(
      installed_dir_.GetPath(), "foo", "https://foo.com"));

  EXPECT_TRUE(GetSites(facade).empty());
}

TEST_F(SponsoredSitesFacadeTest, ReturnsSitesAlreadyVisited) {
  std::unique_ptr<history::HistoryService> history_service =
      CreateHistoryServiceWithVisit(GURL("https://foo.com"));

  SponsoredSitesFacade facade(profile_prefs_, background_images_service_.get(),
                              history_service.get());
  background_images_service_->OnGetSponsoredSitesData(CreateSponsoredSitesData(
      installed_dir_.GetPath(), "foo", "https://foo.com"));

  const std::vector<mojom::SponsoredSitePtr> sites = GetSites(facade);
  ASSERT_EQ(1u, sites.size());
  EXPECT_EQ("foo", sites[0]->title);
}

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
TEST_F(SponsoredSitesFacadeTest, ReturnsNoSitesWhenRewardsWalletConnected) {
  profile_prefs_.SetString(brave_rewards::prefs::kExternalWalletType, "uphold");

  std::unique_ptr<history::HistoryService> history_service =
      CreateHistoryServiceWithVisit(GURL("https://foo.com"));

  SponsoredSitesFacade facade(profile_prefs_, background_images_service_.get(),
                              history_service.get());
  background_images_service_->OnGetSponsoredSitesData(CreateSponsoredSitesData(
      installed_dir_.GetPath(), "foo", "https://foo.com"));

  EXPECT_TRUE(GetSites(facade).empty());
}
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

TEST_F(SponsoredSitesFacadeTest, InvokesCallbackWhenSponsoredSitesDataUpdates) {
  SponsoredSitesFacade facade(profile_prefs_, background_images_service_.get(),
                              /*history_service=*/nullptr);

  bool callback_invoked = false;
  facade.SetSitesUpdatedCallback(base::BindLambdaForTesting(
      [&callback_invoked] { callback_invoked = true; }));

  background_images_service_->OnGetSponsoredSitesData(CreateSponsoredSitesData(
      installed_dir_.GetPath(), "foo", "https://foo.com"));

  EXPECT_TRUE(callback_invoked);
}

}  // namespace brave_new_tab_page_refresh
