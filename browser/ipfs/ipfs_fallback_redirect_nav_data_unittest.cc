/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_fallback_redirect_nav_data.h"

#include <algorithm>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/test/bind.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/navigation_simulator.h"
#include "content/test/test_web_contents.h"
#include "services/network/test/test_network_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ipfs {

class IpfsFallbackRedirectNavigationDataUnitTest : public testing::Test {
 public:
  IpfsFallbackRedirectNavigationDataUnitTest()
      : profile_manager_(TestingBrowserProcess::GetGlobal()) {}
  ~IpfsFallbackRedirectNavigationDataUnitTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(profile_manager_.SetUp(temp_dir_.GetPath()));
    test_network_context_ = std::make_unique<network::TestNetworkContext>();
    profile_ = profile_manager_.CreateTestingProfile("TestProfile");
    web_contents_ = content::TestWebContents::Create(profile(), nullptr);
    ASSERT_TRUE(web_contents_.get());
  }
  TestingProfile* profile() { return profile_; }
  content::TestWebContents* web_contents() { return web_contents_.get(); }

  void NavigateAndCommit(const GURL& url) {
    std::unique_ptr<content::NavigationSimulator> navigation =
        content::NavigationSimulator::CreateBrowserInitiated(
            url, web_contents_.get());
    navigation->Commit();
    navigation->Wait();
  }

  void DeleteAllEntriesExceptLastCommitted() {
    web_contents()->GetController().DeleteNavigationEntries(base::BindRepeating(
        [](content::NavigationEntry* entry) { return true; }));
  }

  IpfsFallbackRedirectNavigationData* SetUserDataForNavEntry(
      const int& entry_index) {
    return IpfsFallbackRedirectNavigationData::GetOrCreate(
        web_contents()->GetController().GetEntryAtIndex(entry_index));
  }

  void CallForEachEntry(
      base::RepeatingCallback<void(content::NavigationEntry*)> callback) {
    auto& controller = web_contents()->GetController();
    for (int i = 0; i < controller.GetEntryCount(); i++) {
      auto* entry = controller.GetEntryAtIndex(i);
      if (!entry) {
        continue;
      }
      callback.Run(entry);
    }
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler render_view_host_test_enabler_;
  base::ScopedTempDir temp_dir_;
  TestingProfileManager profile_manager_;
  raw_ptr<TestingProfile> profile_ = nullptr;
  std::unique_ptr<content::TestWebContents> web_contents_;
  std::unique_ptr<network::TestNetworkContext> test_network_context_;
};

TEST_F(IpfsFallbackRedirectNavigationDataUnitTest,
       CleanUserDataForAllNavEntries) {
  NavigateAndCommit(GURL("http://address.adr"));
  NavigateAndCommit(GURL("http://address12adr"));
  NavigateAndCommit(GURL("http://address1.adr"));

  EXPECT_EQ(web_contents()->GetController().GetEntryCount(), 3);

  auto* entry0 = SetUserDataForNavEntry(0);
  EXPECT_NE(entry0, nullptr);
  EXPECT_TRUE(entry0->GetOriginalUrl().is_empty());
  EXPECT_FALSE(entry0->IsAutoRedirectBlocked());

  auto* entry2 = SetUserDataForNavEntry(1);
  EXPECT_NE(entry2, nullptr);
  entry2->SetRemoveFlag(true);

  auto* entry1 = SetUserDataForNavEntry(2);
  EXPECT_NE(entry1, nullptr);
  EXPECT_TRUE(entry1->GetOriginalUrl().is_empty());
  EXPECT_FALSE(entry1->IsAutoRedirectBlocked());


  IpfsFallbackRedirectNavigationData::CleanAll(web_contents());
  EXPECT_EQ(web_contents()->GetController().GetEntryCount(), 2);
  CallForEachEntry(
      base::BindLambdaForTesting([](content::NavigationEntry* entry) {
        auto* current_entry_data =
            IpfsFallbackRedirectNavigationData::GetFallbackData(entry);
        EXPECT_EQ(current_entry_data, nullptr);
      }));
}

TEST_F(IpfsFallbackRedirectNavigationDataUnitTest,
       GetFallbackDataFromRedirectChain) {
  auto test_action =
      [&](std::vector<std::tuple<
              GURL, absl::optional<IpfsFallbackRedirectNavigationData>>>
              entries,
          base::RepeatingCallback<void()> check_callback,
          const std::string error_msg = "") {
        std::for_each(
            entries.begin(), entries.end(),
            [&](const std::tuple<
                GURL, absl::optional<IpfsFallbackRedirectNavigationData>>&
                    item) {
              NavigateAndCommit(std::get<0>(item));
              if (std::get<1>(item).has_value()) {
                auto* user_data =
                    IpfsFallbackRedirectNavigationData::GetOrCreate(
                        web_contents()
                            ->GetController()
                            .GetLastCommittedEntry());
                user_data->SetOriginalUrl(std::get<1>(item)->GetOriginalUrl());
                user_data->SetAutoRedirectBlock(
                    std::get<1>(item)->IsAutoRedirectBlocked());
                user_data->SetRemoveFlag(std::get<1>(item)->GetRemoveFlag());
              }
            });
        if (web_contents()->GetController().GetEntryCount() > 0) {
          web_contents()->GetController().RemoveEntryAtIndex(0);
        }
        EXPECT_EQ(web_contents()->GetController().GetEntryCount(),
                  entries.empty() ? 1 : static_cast<int>(entries.size()));

        check_callback.Run();

        NavigateAndCommit(GURL("about:blank"));

        DeleteAllEntriesExceptLastCommitted();
      };

  NavigateAndCommit(GURL("about:blank"));

  test_action(
      {{GURL("https://drweb.link/ipns/"
             "k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4/"),
        IpfsFallbackRedirectNavigationData(
            GURL("https://drweb.link/ipns/"
                 "k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4/"),
            false, true)}},
      base::BindLambdaForTesting([&]() {
        auto* nav_data = IpfsFallbackRedirectNavigationData::
            FindFallbackData(web_contents());
        EXPECT_NE(nav_data, nullptr);
        EXPECT_EQ(
            nav_data->GetOriginalUrl(),
            GURL("https://drweb.link/ipns/"
                 "k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4/"));
        EXPECT_EQ(nav_data->IsAutoRedirectBlocked(), false);
        EXPECT_EQ(nav_data->GetRemoveFlag(), true);
      }));
  test_action(
      {{GURL("https://drweb.link/ipns/"
             "k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4/"),
        IpfsFallbackRedirectNavigationData(
            GURL("https://drweb.link/ipns/"
                 "k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4/"),
            false, true)},
       {GURL("ipns://k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4"),
        absl::optional<IpfsFallbackRedirectNavigationData>()}},
      base::BindLambdaForTesting([&]() {
        auto* nav_data = IpfsFallbackRedirectNavigationData::
            FindFallbackData(web_contents());
        EXPECT_NE(nav_data, nullptr);
        EXPECT_EQ(
            nav_data->GetOriginalUrl(),
            GURL("https://drweb.link/ipns/"
                 "k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4/"));
        EXPECT_EQ(nav_data->IsAutoRedirectBlocked(), false);
        EXPECT_EQ(nav_data->GetRemoveFlag(), true);
      }));

  test_action(
      {{GURL("https://drweb.link/ipns/"
             "k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4/"),
        IpfsFallbackRedirectNavigationData(
            GURL("https://drweb.link/ipns/"
                 "k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4/"),
            false, false)},
       {GURL("ipns://k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4"),
        absl::optional<IpfsFallbackRedirectNavigationData>()}},
      base::BindLambdaForTesting([&]() {
        auto* nav_data = IpfsFallbackRedirectNavigationData::
            FindFallbackData(web_contents());
        EXPECT_NE(nav_data, nullptr);
      }));
}

}  // namespace ipfs
