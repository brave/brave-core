// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/brave_p3a_message_manager.h"

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/time/time.h"
#include "base/time/time_to_iso8601.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"
#include "brave/components/p3a/brave_p3a_config.h"
#include "brave/components/p3a/brave_p3a_service.h"
#include "brave/components/p3a/metric_names.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/p3a/star_randomness_test_util.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

namespace {

constexpr uint8_t kInitialEpoch = 5;
constexpr size_t kUploadIntervalSeconds = 120;
constexpr size_t kEpochLenDays = 4;
constexpr char kTestHost[] = "https://localhost:8443";
constexpr char kP2APrefix[] = "Brave.P2A";

}  // namespace

class P3AMessageManagerTest : public testing::Test {
 public:
  P3AMessageManagerTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory)) {}

 protected:
  void SetUp() override {
    p3a_config.disable_star_attestation = true;
    p3a_config.star_randomness_host = kTestHost;
    p3a_config.randomize_upload_interval = false;
    p3a_config.average_upload_interval = base::Seconds(kUploadIntervalSeconds);
    p3a_config.p3a_json_upload_url = GURL(std::string(kTestHost) + "/p3a_json");
    p3a_config.p2a_json_upload_url = GURL(std::string(kTestHost) + "/p2a_json");
    p3a_config.p3a_star_upload_url = GURL(std::string(kTestHost) + "/p3a_star");
    p3a_config.p2a_star_upload_url = GURL(std::string(kTestHost) + "/p2a_star");

    BraveP3AService::RegisterPrefs(local_state.registry(), true);
    RegisterPrefsForBraveReferralsService(local_state.registry());

    next_epoch_time = base::Time::Now() + base::Days(kEpochLenDays);

    url_loader_factory.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory.ClearResponses();

          if (request.url == GURL(std::string(kTestHost) + "/info")) {
            EXPECT_EQ(request.method, net::HttpRequestHeaders::kGetMethod);
            url_loader_factory.AddResponse(
                request.url.spec(),
                "{\"currentEpoch\":" + base::NumberToString(current_epoch) +
                    ", \"nextEpochTime\": \"" + TimeToISO8601(next_epoch_time) +
                    "\"}");
            info_request_made = true;
          } else if (request.url ==
                     GURL(std::string(kTestHost) + "/randomness")) {
            std::string resp_json =
                HandleRandomnessRequest(request, current_epoch);
            url_loader_factory.AddResponse(request.url.spec(), resp_json);

            points_requests_made++;
          } else if (request.url == p3a_config.p3a_json_upload_url) {
            EXPECT_EQ(request.method, net::HttpRequestHeaders::kPostMethod);
            StoreJsonMetricInMap(request, false);
            url_loader_factory.AddResponse(request.url.spec(), "{}");
          } else if (request.url == p3a_config.p2a_json_upload_url) {
            EXPECT_EQ(request.method, net::HttpRequestHeaders::kPostMethod);
            StoreJsonMetricInMap(request, true);
            url_loader_factory.AddResponse(request.url.spec(), "{}");
          } else if (request.url == p3a_config.p3a_star_upload_url) {
            EXPECT_EQ(request.method, net::HttpRequestHeaders::kPostMethod);
            std::string message = std::string(ExtractBodyFromRequest(request));
            EXPECT_EQ(p3a_star_sent_messages.find(message),
                      p3a_star_sent_messages.end());
            p3a_star_sent_messages.insert(message);
            url_loader_factory.AddResponse(request.url.spec(), "{}");
          } else if (request.url == p3a_config.p2a_star_upload_url) {
            EXPECT_EQ(request.method, net::HttpRequestHeaders::kPostMethod);
            std::string message = std::string(ExtractBodyFromRequest(request));
            EXPECT_EQ(p2a_star_sent_messages.find(message),
                      p2a_star_sent_messages.end());
            p2a_star_sent_messages.insert(message);
            url_loader_factory.AddResponse(request.url.spec(), "{}");
          }
        }));

    message_manager.reset(new BraveP3AMessageManager(&local_state, &p3a_config,
                                                     "release", "2022-01-01"));

    message_manager->Init(shared_url_loader_factory);

    task_environment_.RunUntilIdle();
  }

  void ResetInterceptorStores() {
    p3a_json_sent_metrics.clear();
    p2a_json_sent_metrics.clear();
    p3a_star_sent_messages.clear();
    p2a_star_sent_messages.clear();
    info_request_made = false;
    points_requests_made = 0;
  }

  std::vector<std::string> GetTestHistogramNames(size_t p3a_count,
                                                 size_t p2a_count) {
    std::vector<std::string> result;
    size_t p3a_i = 0;
    size_t p2a_i = 0;
    for (const base::StringPiece& histogram_name : p3a::kCollectedHistograms) {
      if (histogram_name.rfind(kP2APrefix, 0) == 0) {
        if (p2a_i < p2a_count) {
          result.push_back(std::string(histogram_name));
          p2a_i++;
        }
      } else if (p3a_i < p3a_count) {
        result.push_back(std::string(histogram_name));
        p3a_i++;
      }

      if (p2a_i >= p2a_count && p3a_i >= p3a_count) {
        break;
      }
    }
    return result;
  }

  content::BrowserTaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory;
  BraveP3AConfig p3a_config;
  std::unique_ptr<BraveP3AMessageManager> message_manager;
  TestingPrefServiceSimple local_state;

  std::map<std::string, size_t> p3a_json_sent_metrics;
  std::map<std::string, size_t> p2a_json_sent_metrics;

  std::set<std::string> p3a_star_sent_messages;
  std::set<std::string> p2a_star_sent_messages;

  bool info_request_made = false;
  size_t points_requests_made = 0;

  uint8_t current_epoch = kInitialEpoch;
  base::Time next_epoch_time;

 private:
  base::StringPiece ExtractBodyFromRequest(
      const network::ResourceRequest& request) {
    return request.request_body->elements()
        ->at(0)
        .As<network::DataElementBytes>()
        .AsStringPiece();
  }

  void StoreJsonMetricInMap(const network::ResourceRequest& request,
                            bool is_p2a) {
    base::StringPiece body = ExtractBodyFromRequest(request);
    base::Value parsed_log = *base::JSONReader::Read(body);
    std::string metric_name = *parsed_log.FindStringKey("metric_name");
    int metric_value = *parsed_log.FindIntKey("metric_value");

    if (is_p2a) {
      EXPECT_EQ(p2a_json_sent_metrics.find(metric_name),
                p2a_json_sent_metrics.end());
      p2a_json_sent_metrics[metric_name] = metric_value;
    } else {
      EXPECT_EQ(p3a_json_sent_metrics.find(metric_name),
                p3a_json_sent_metrics.end());
      p3a_json_sent_metrics[metric_name] = metric_value;
    }
  }
};

TEST_F(P3AMessageManagerTest, UpdateLogsAndSendJson) {
  std::vector<std::string> test_histograms = GetTestHistogramNames(3, 4);

  for (size_t i = 0; i < test_histograms.size(); i++) {
    message_manager->UpdateMetricValue(test_histograms[i], i + 1);
  }

  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 50));

  EXPECT_EQ(p3a_json_sent_metrics.size(), 3U);
  EXPECT_EQ(p2a_json_sent_metrics.size(), 4U);

  for (size_t i = 0; i < test_histograms.size(); i++) {
    if (test_histograms[i].rfind(kP2APrefix, 0) == 0) {
      ASSERT_EQ(p2a_json_sent_metrics.find(test_histograms[i])->second, i + 1);
    } else {
      ASSERT_EQ(p3a_json_sent_metrics.find(test_histograms[i])->second, i + 1);
    }
  }

  ResetInterceptorStores();
  task_environment_.FastForwardBy(base::Days(7) +
                                  base::Seconds(kUploadIntervalSeconds * 50));

  EXPECT_EQ(p3a_json_sent_metrics.size(), 3U);
  EXPECT_EQ(p2a_json_sent_metrics.size(), 4U);
  for (size_t i = 0; i < test_histograms.size(); i++) {
    if (test_histograms[i].rfind(kP2APrefix, 0) == 0) {
      ASSERT_EQ(p2a_json_sent_metrics.find(test_histograms[i])->second, i + 1);
    } else {
      ASSERT_EQ(p3a_json_sent_metrics.find(test_histograms[i])->second, i + 1);
    }
  }
}

TEST_F(P3AMessageManagerTest, UpdateLogsAndSendStar) {
  ASSERT_TRUE(info_request_made);

  std::vector<std::string> test_histograms = GetTestHistogramNames(4, 3);

  for (size_t i = 0; i < test_histograms.size(); i++) {
    message_manager->UpdateMetricValue(test_histograms[i], i + 1);
  }

  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 100));

  EXPECT_EQ(points_requests_made, 7U);
  // Should not send metrics, since they are in current epoch
  EXPECT_EQ(p3a_star_sent_messages.size(), 0U);
  EXPECT_EQ(p2a_star_sent_messages.size(), 0U);

  ResetInterceptorStores();
  current_epoch++;
  next_epoch_time += base::Days(kEpochLenDays);
  task_environment_.FastForwardBy(base::Days(kEpochLenDays) +
                                  base::Seconds(kUploadIntervalSeconds * 100));

  ASSERT_TRUE(info_request_made);
  EXPECT_EQ(points_requests_made, 7U);
  EXPECT_EQ(p3a_star_sent_messages.size(), 4U);
  EXPECT_EQ(p2a_star_sent_messages.size(), 3U);

  ResetInterceptorStores();
  current_epoch++;
  next_epoch_time += base::Days(kEpochLenDays);
  task_environment_.FastForwardBy(base::Days(kEpochLenDays) +
                                  base::Seconds(kUploadIntervalSeconds * 100));

  ASSERT_TRUE(info_request_made);
  EXPECT_EQ(points_requests_made, 7U);
  EXPECT_EQ(p3a_star_sent_messages.size(), 4U);
  EXPECT_EQ(p2a_star_sent_messages.size(), 3U);
}

TEST_F(P3AMessageManagerTest, DoesNotSendRemovedMetricValue) {
  std::vector<std::string> test_histograms = GetTestHistogramNames(3, 3);

  for (const std::string& histogram_name : test_histograms) {
    message_manager->UpdateMetricValue(histogram_name, 5);
  }

  for (const std::string& histogram_name : test_histograms) {
    message_manager->RemoveMetricValue(histogram_name);
  }

  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 100));

  EXPECT_EQ(points_requests_made, 0U);
  EXPECT_EQ(p3a_json_sent_metrics.size(), 0U);
  EXPECT_EQ(p2a_json_sent_metrics.size(), 0U);

  current_epoch++;
  next_epoch_time += base::Days(kEpochLenDays);
  task_environment_.FastForwardBy(base::Days(kEpochLenDays) +
                                  base::Seconds(kUploadIntervalSeconds * 100));

  EXPECT_EQ(points_requests_made, 0U);
  EXPECT_EQ(p3a_star_sent_messages.size(), 0U);
  EXPECT_EQ(p2a_star_sent_messages.size(), 0U);
}

TEST_F(P3AMessageManagerTest, ShouldNotSendIfDisabled) {
  std::vector<std::string> test_histograms = GetTestHistogramNames(3, 3);

  for (const std::string& histogram_name : test_histograms) {
    message_manager->UpdateMetricValue(histogram_name, 5);
  }

  local_state.SetBoolean(kP3AEnabled, false);

  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 100));

  EXPECT_EQ(points_requests_made, 0U);
  EXPECT_EQ(p3a_json_sent_metrics.size(), 0U);
  EXPECT_EQ(p2a_json_sent_metrics.size(), 0U);
  EXPECT_EQ(p3a_star_sent_messages.size(), 0U);
  EXPECT_EQ(p2a_star_sent_messages.size(), 0U);

  current_epoch++;
  next_epoch_time += base::Days(kEpochLenDays);
  task_environment_.FastForwardBy(base::Days(kEpochLenDays) +
                                  base::Seconds(kUploadIntervalSeconds * 100));

  EXPECT_EQ(points_requests_made, 0U);
  EXPECT_EQ(p3a_star_sent_messages.size(), 0U);
  EXPECT_EQ(p2a_star_sent_messages.size(), 0U);
}

}  // namespace brave
