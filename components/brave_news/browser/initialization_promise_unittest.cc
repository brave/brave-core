// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/initialization_promise.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/urls.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

constexpr char kPublishersResponse[] = R"([
    {
        "publisher_id": "111",
        "publisher_name": "Test Publisher 1",
        "feed_url": "https://tp1.example.com/feed",
        "site_url": "https://tp1.example.com",
        "category": "Tech",
        "cover_url": "https://tp1.example.com/cover",
        "cover_url": "https://tp1.example.com/favicon",
        "background_color": "#FF0000",
        "locales": [{
          "locale": "en_NZ",
          "channels": ["One", "Tech"]
        }],
        "enabled": false
    }])";

class BraveNewsInitializationPromiseTest : public testing::Test {
 public:
  BraveNewsInitializationPromiseTest()
      : api_request_helper_(TRAFFIC_ANNOTATION_FOR_TESTS,
                            test_url_loader_factory_.GetSafeWeakWrapper()) {
    prefs::RegisterProfilePrefs(pref_service_.registry());

    pref_manager_ = std::make_unique<BraveNewsPrefManager>(pref_service_);
    // Ensure Brave News is enabled.
    pref_manager_->SetConfig(mojom::Configuration::New(true, true, true));

    publishers_controller_ =
        std::make_unique<PublishersController>(&api_request_helper_);

    initialization_promise_ = std::make_unique<InitializationPromise>(
        3, *pref_manager_,
        base::BindRepeating(&PublishersController::GetLocale,
                            base::Unretained(publishers_controller_.get()),
                            pref_manager_->GetSubscriptions()));

    // Disable backoffs, so we can test.
    initialization_promise_->set_no_retry_delay_for_testing(true);
  }
  ~BraveNewsInitializationPromiseTest() override = default;

 protected:
  std::string InitializeAndGetLocale() {
    base::RunLoop loop;
    initialization_promise_->OnceInitialized(loop.QuitClosure());
    loop.Run();

    auto channels = pref_manager_->GetSubscriptions().channels();
    if (channels.empty()) {
      return "";
    }
    return channels.begin()->first;
  }

  static std::string GetSourcesUrl() {
    return "https://" + brave_news::GetHostname() + "/sources." +
           brave_news::kRegionUrlPart + "json";
  }

  void SucceedAfter(size_t retries) {
    test_url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, retries](const network::ResourceRequest& request) {
          bool should_succeed =
              initialization_promise_->attempts_for_testing() >= retries;
          test_url_loader_factory_.AddResponse(
              request.url.spec(), should_succeed ? kPublishersResponse : "",
              should_succeed ? net::HTTP_OK : net::HTTP_SERVICE_UNAVAILABLE);
        }));
  }

  content::BrowserTaskEnvironment browser_task_environment_;
  data_decoder::test::InProcessDataDecoder data_decoder_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  api_request_helper::APIRequestHelper api_request_helper_;

  sync_preferences::TestingPrefServiceSyncable pref_service_;
  std::unique_ptr<BraveNewsPrefManager> pref_manager_;
  std::unique_ptr<PublishersController> publishers_controller_;
  std::unique_ptr<InitializationPromise> initialization_promise_;

 private:
  const brave_l10n::test::ScopedDefaultLocale locale_{"en_NZ"};
};

TEST_F(BraveNewsInitializationPromiseTest,
       WaitingForInitializationChangesState) {
  EXPECT_EQ(InitializationPromise::State::kNone,
            initialization_promise_->state());
  initialization_promise_->OnceInitialized(base::DoNothing());
  EXPECT_EQ(InitializationPromise::State::kInitializing,
            initialization_promise_->state());
}

TEST_F(BraveNewsInitializationPromiseTest, InitializedLocaleIsCorrect) {
  SucceedAfter(0);
  auto locale = InitializeAndGetLocale();
  EXPECT_EQ("en_NZ", locale);
  EXPECT_EQ(1u, initialization_promise_->attempts_for_testing());
}

TEST_F(BraveNewsInitializationPromiseTest, InitializationRetries) {
  SucceedAfter(1);
  auto locale = InitializeAndGetLocale();
  EXPECT_EQ("en_NZ", locale);
  EXPECT_TRUE(initialization_promise_->complete());
  EXPECT_FALSE(initialization_promise_->failed());
  EXPECT_EQ(2u, initialization_promise_->attempts_for_testing());
}

TEST_F(BraveNewsInitializationPromiseTest,
       InitializationDoesNotRetryFourTimes) {
  SucceedAfter(4);
  auto locale = InitializeAndGetLocale();
  EXPECT_EQ("", locale);
  EXPECT_TRUE(initialization_promise_->complete());
  EXPECT_TRUE(initialization_promise_->failed());
  EXPECT_EQ(3u, initialization_promise_->attempts_for_testing());
}

}  // namespace brave_news
