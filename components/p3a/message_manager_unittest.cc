// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/message_manager.h"

#include <map>
#include <memory>
#include <set>
#include <string_view>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "brave/components/p3a/features.h"
#include "brave/components/p3a/metric_names.h"
#include "brave/components/p3a/p3a_config.h"
#include "brave/components/p3a/p3a_service.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/p3a/star_randomness_test_util.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

namespace {

constexpr uint8_t kInitialEpoch = 2;
constexpr size_t kUploadIntervalSeconds = 120;
constexpr size_t kEpochLenDays = 4;
constexpr char kTestHost[] = "https://localhost:8443";
constexpr char kP2APrefix[] = "Brave.P2A";

}  // namespace

class P3AMessageManagerTest : public testing::Test,
                              public MessageManager::Delegate {
 public:
  P3AMessageManagerTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory)) {}

  absl::optional<MetricLogType> GetDynamicMetricLogType(
      const std::string& histogram_name) const override {
    return absl::optional<MetricLogType>();
  }

  void OnRotation(MetricLogType log_type, bool is_constellation) override {}

  void OnMetricCycled(const std::string& histogram_name,
                      bool is_constellation) override {}

 protected:
  void SetUpManager(bool is_constellation_enabled) {
    if (is_constellation_enabled) {
      scoped_feature_list_.InitWithFeatures(
          {features::kConstellation,
           features::kConstellationEnclaveAttestation},
          {});
    }

    base::Time future_mock_time;
    if (base::Time::FromString("2050-01-04", &future_mock_time)) {
      task_environment_.AdvanceClock(future_mock_time - base::Time::Now());
    }

    p3a_config.disable_star_attestation = true;
    p3a_config.star_randomness_host = kTestHost;
    p3a_config.randomize_upload_interval = false;
    p3a_config.average_upload_interval = base::Seconds(kUploadIntervalSeconds);
    p3a_config.p3a_json_upload_url = GURL(std::string(kTestHost) + "/p3a_json");
    p3a_config.p2a_json_upload_url = GURL(std::string(kTestHost) + "/p2a_json");
    p3a_config.p3a_constellation_upload_url =
        GURL(std::string(kTestHost) + "/p3a_constellation");

    P3AService::RegisterPrefs(local_state.registry(), true);

    next_epoch_time = base::Time::Now() + base::Days(kEpochLenDays);

    url_loader_factory.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory.ClearResponses();

          if (request.url == GURL(std::string(kTestHost) + "/info") ||
              request.url == GURL(std::string(kTestHost) + "/randomness")) {
            if (interceptor_invalid_response_from_randomness) {
              // next epoch time is missing!
              url_loader_factory.AddResponse(
                  request.url.spec(),
                  "{\"currentEpoch\":" + base::NumberToString(current_epoch) +
                      "}");
              return;
            } else if (interceptor_invalid_response_from_randomness_non_json) {
              url_loader_factory.AddResponse(
                  request.url.spec(), "invalid response that is not json");
              return;
            }
          }

          if (request.url == GURL(std::string(kTestHost) + "/info")) {
            EXPECT_EQ(request.method, net::HttpRequestHeaders::kGetMethod);
            url_loader_factory.AddResponse(
                request.url.spec(),
                "{\"currentEpoch\":" + base::NumberToString(current_epoch) +
                    ", \"nextEpochTime\": \"" +
                    TimeFormatAsIso8601(next_epoch_time) + "\"}",
                interceptor_status_code_from_randomness);
            info_request_made = true;
          } else if (request.url ==
                     GURL(std::string(kTestHost) + "/randomness")) {
            std::string resp_json =
                HandleRandomnessRequest(request, current_epoch);
            url_loader_factory.AddResponse(
                request.url.spec(), resp_json,
                interceptor_status_code_from_randomness);

            points_requests_made++;
          } else if (request.url == p3a_config.p3a_json_upload_url) {
            EXPECT_EQ(request.method, net::HttpRequestHeaders::kPostMethod);
            StoreJsonMetricInMap(request, false);
            url_loader_factory.AddResponse(request.url.spec(), "{}");
          } else if (request.url == p3a_config.p2a_json_upload_url) {
            EXPECT_EQ(request.method, net::HttpRequestHeaders::kPostMethod);
            StoreJsonMetricInMap(request, true);
            url_loader_factory.AddResponse(request.url.spec(), "{}");
          } else if (request.url == p3a_config.p3a_constellation_upload_url) {
            EXPECT_EQ(request.method, net::HttpRequestHeaders::kPostMethod);
            std::string message = std::string(ExtractBodyFromRequest(request));
            EXPECT_EQ(p3a_constellation_sent_messages.find(message),
                      p3a_constellation_sent_messages.end());
            p3a_constellation_sent_messages.insert(message);
            url_loader_factory.AddResponse(request.url.spec(), "{}");
          }
        }));

    message_manager = std::make_unique<MessageManager>(
        local_state, &p3a_config, *this, "release", "2099-01-01");

    message_manager->Init(shared_url_loader_factory);

    task_environment_.RunUntilIdle();
  }

  void ResetInterceptorStores() {
    p3a_json_sent_metrics.clear();
    p2a_json_sent_metrics.clear();
    p3a_constellation_sent_messages.clear();
    info_request_made = false;
    points_requests_made = 0;
  }

  std::vector<std::string> GetTestHistogramNames(size_t p3a_count,
                                                 size_t p2a_count) {
    std::vector<std::string> result;
    size_t p3a_i = 0;
    size_t p2a_i = 0;
    for (const std::string_view histogram_name :
         p3a::kCollectedTypicalHistograms) {
      if (histogram_name.rfind(kP2APrefix, 0) == 0) {
        if (p2a_i < p2a_count) {
          result.push_back(std::string(histogram_name));
          p2a_i++;
        }
      } else if (p3a_i < p3a_count &&
                 base::StartsWith(histogram_name, "Brave.Core")) {
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
  base::test::ScopedFeatureList scoped_feature_list_;
  network::TestURLLoaderFactory url_loader_factory;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory;
  P3AConfig p3a_config;
  std::unique_ptr<MessageManager> message_manager;
  TestingPrefServiceSimple local_state;

  std::map<std::string, size_t> p3a_json_sent_metrics;
  std::map<std::string, size_t> p2a_json_sent_metrics;

  std::set<std::string> p3a_constellation_sent_messages;

  bool interceptor_invalid_response_from_randomness = false;
  bool interceptor_invalid_response_from_randomness_non_json = false;
  net::HttpStatusCode interceptor_status_code_from_randomness = net::HTTP_OK;
  bool info_request_made = false;
  size_t points_requests_made = 0;

  uint8_t current_epoch = kInitialEpoch;
  base::Time next_epoch_time;

 private:
  std::string_view ExtractBodyFromRequest(
      const network::ResourceRequest& request) {
    return request.request_body->elements()
        ->at(0)
        .As<network::DataElementBytes>()
        .AsStringPiece();
  }

  void StoreJsonMetricInMap(const network::ResourceRequest& request,
                            bool is_p2a) {
    std::string_view body = ExtractBodyFromRequest(request);
    base::Value::Dict parsed_log = base::test::ParseJsonDict(body);
    std::string* metric_name = parsed_log.FindString("metric_name");
    ASSERT_TRUE(metric_name);
    absl::optional<int> metric_value = parsed_log.FindInt("metric_value");
    ASSERT_TRUE(metric_value);

    if (is_p2a) {
      EXPECT_EQ(p2a_json_sent_metrics.find(*metric_name),
                p2a_json_sent_metrics.end());
      p2a_json_sent_metrics[*metric_name] = *metric_value;
    } else {
      EXPECT_EQ(p3a_json_sent_metrics.find(*metric_name),
                p3a_json_sent_metrics.end());
      p3a_json_sent_metrics[*metric_name] = *metric_value;
    }
  }
};

TEST_F(P3AMessageManagerTest, UpdateLogsAndSendJson) {
  SetUpManager(true);
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

TEST_F(P3AMessageManagerTest, UpdateLogsAndDontSendConstellation) {
  // don't perform any constellation activity if feature is disabled
  SetUpManager(false);
  ASSERT_FALSE(info_request_made);

  std::vector<std::string> test_histograms = GetTestHistogramNames(7, 0);

  for (size_t i = 0; i < test_histograms.size(); i++) {
    message_manager->UpdateMetricValue(test_histograms[i], i + 1);
  }

  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 100));

  EXPECT_EQ(points_requests_made, 0U);
  EXPECT_EQ(p3a_constellation_sent_messages.size(), 0U);

  current_epoch++;
  next_epoch_time += base::Days(kEpochLenDays);
  task_environment_.FastForwardBy(base::Days(kEpochLenDays) +
                                  base::Seconds(kUploadIntervalSeconds * 100));

  ASSERT_FALSE(info_request_made);
  EXPECT_EQ(points_requests_made, 0U);
  EXPECT_EQ(p3a_constellation_sent_messages.size(), 0U);
}

TEST_F(P3AMessageManagerTest, UpdateLogsAndSendConstellation) {
  SetUpManager(true);
  ASSERT_TRUE(info_request_made);

  std::vector<std::string> test_histograms = GetTestHistogramNames(7, 0);

  for (size_t i = 0; i < test_histograms.size(); i++) {
    message_manager->UpdateMetricValue(test_histograms[i], i + 1);
  }

  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 100));

  EXPECT_EQ(points_requests_made, 7U);
  EXPECT_EQ(p3a_constellation_sent_messages.size(), 7U);

  ResetInterceptorStores();
  current_epoch++;
  next_epoch_time += base::Days(kEpochLenDays);
  task_environment_.FastForwardBy(base::Days(kEpochLenDays) +
                                  base::Seconds(kUploadIntervalSeconds * 100));

  ASSERT_TRUE(info_request_made);
  EXPECT_EQ(points_requests_made, 7U);
  EXPECT_EQ(p3a_constellation_sent_messages.size(), 7U);

  ResetInterceptorStores();
  current_epoch++;
  next_epoch_time += base::Days(kEpochLenDays);
  task_environment_.FastForwardBy(base::Days(kEpochLenDays) +
                                  base::Seconds(kUploadIntervalSeconds * 100));

  ASSERT_TRUE(info_request_made);
  EXPECT_EQ(points_requests_made, 7U);
  EXPECT_EQ(p3a_constellation_sent_messages.size(), 7U);
}

TEST_F(P3AMessageManagerTest, UpdateLogsAndSendConstellationInvalidResponse) {
  SetUpManager(true);
  ASSERT_TRUE(info_request_made);

  std::vector<std::string> test_histograms = GetTestHistogramNames(7, 0);

  for (size_t i = 0; i < test_histograms.size(); i++) {
    message_manager->UpdateMetricValue(test_histograms[i], i + 1);
  }

  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 100));
  ResetInterceptorStores();

  // server will return invalid response body that is json, but has missing
  // fields
  interceptor_invalid_response_from_randomness = true;

  // skip ahead to next epoch
  current_epoch++;
  next_epoch_time += base::Days(kEpochLenDays);
  task_environment_.FastForwardBy(base::Days(kEpochLenDays));

  // We are at the beginning of the new epoch. measurements from previous epoch
  // should not be sent since we are unable to get the current epoch from the
  // server.
  EXPECT_EQ(p3a_constellation_sent_messages.size(), 0U);

  // server will return response body that is not json
  interceptor_invalid_response_from_randomness = false;
  interceptor_invalid_response_from_randomness_non_json = true;

  current_epoch++;
  next_epoch_time += base::Days(kEpochLenDays);
  task_environment_.FastForwardBy(base::Days(kEpochLenDays));

  // no new measurements should have been recorded in the previous epoch.
  EXPECT_EQ(p3a_constellation_sent_messages.size(), 0U);

  // restore randomness server functionality
  interceptor_invalid_response_from_randomness_non_json = false;

  current_epoch++;
  next_epoch_time += base::Days(kEpochLenDays);
  task_environment_.FastForwardBy(base::Days(kEpochLenDays));

  // randomness server is now providing correct response . no new measurements
  // should have been recorded in the previous epoch due to previous
  // unavailability. randomness points should be requested for the current
  // epoch. messages from the first epoch should be sent.
  ASSERT_TRUE(info_request_made);
  EXPECT_EQ(points_requests_made, 7U);
  EXPECT_EQ(p3a_constellation_sent_messages.size(), 7U);
}

TEST_F(P3AMessageManagerTest,
       UpdateLogsAndSendConstellationInvalidClientRequest) {
  SetUpManager(true);
  ASSERT_TRUE(info_request_made);

  std::vector<std::string> test_histograms = GetTestHistogramNames(7, 0);

  for (size_t i = 0; i < test_histograms.size(); i++) {
    message_manager->UpdateMetricValue(test_histograms[i], i + 1);
  }

  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 100));
  ResetInterceptorStores();

  // server will return HTTP 500 to indicate an invalid client request.
  interceptor_status_code_from_randomness = net::HTTP_BAD_REQUEST;

  // skip ahead to next epoch
  current_epoch++;
  next_epoch_time += base::Days(kEpochLenDays);
  task_environment_.FastForwardBy(base::Days(kEpochLenDays));

  // We are at the beginning of the new epoch. measurements from previous epoch
  // should not be sent since we are unable to get the current epoch from the
  // server.
  EXPECT_EQ(p3a_constellation_sent_messages.size(), 0U);

  // restore randomness server functionality
  interceptor_status_code_from_randomness = net::HTTP_OK;

  current_epoch++;
  next_epoch_time += base::Days(kEpochLenDays);
  task_environment_.FastForwardBy(base::Days(kEpochLenDays));

  // randomness server is now accepting client request. no new measurements
  // should have been recorded in the previous epoch due to previous
  // unavailability. randomness points should be requested for the current
  // epoch. messages from the first epoch should be sent.
  ASSERT_TRUE(info_request_made);
  EXPECT_EQ(points_requests_made, 7U);
  EXPECT_EQ(p3a_constellation_sent_messages.size(), 7U);
}

TEST_F(P3AMessageManagerTest, UpdateLogsAndSendConstellationUnavailable) {
  SetUpManager(true);
  ASSERT_TRUE(info_request_made);

  std::vector<std::string> test_histograms = GetTestHistogramNames(7, 0);

  for (size_t i = 0; i < test_histograms.size(); i++) {
    message_manager->UpdateMetricValue(test_histograms[i], i + 1);
  }

  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 100));
  ResetInterceptorStores();

  // server will return HTTP 500 to indicate unavailability.
  interceptor_status_code_from_randomness = net::HTTP_INTERNAL_SERVER_ERROR;

  // skip ahead to next epoch
  current_epoch++;
  next_epoch_time += base::Days(kEpochLenDays);
  task_environment_.FastForwardBy(base::Days(kEpochLenDays));

  // We are at the beginning of the new epoch. measurements from previous epoch
  // should not be sent since we are unable to get the current epoch from the
  // server.
  EXPECT_EQ(p3a_constellation_sent_messages.size(), 0U);

  // restore randomness server functionality
  interceptor_status_code_from_randomness = net::HTTP_OK;

  current_epoch++;
  next_epoch_time += base::Days(kEpochLenDays);
  task_environment_.FastForwardBy(base::Days(kEpochLenDays));

  // randomness server is now available. no new measurements should have been
  // recorded in the previous epoch due to previous unavailability.
  // randomness points should be requested for the current epoch.
  // messages from the first epoch should be sent.
  ASSERT_TRUE(info_request_made);
  EXPECT_EQ(points_requests_made, 7U);
  EXPECT_EQ(p3a_constellation_sent_messages.size(), 7U);
}

TEST_F(P3AMessageManagerTest, DoesNotSendRemovedMetricValue) {
  SetUpManager(true);
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
  EXPECT_EQ(p3a_constellation_sent_messages.size(), 0U);
}

TEST_F(P3AMessageManagerTest, ShouldNotSendIfDisabled) {
  SetUpManager(true);
  std::vector<std::string> test_histograms = GetTestHistogramNames(3, 3);

  for (const std::string& histogram_name : test_histograms) {
    message_manager->UpdateMetricValue(histogram_name, 5);
  }

  local_state.SetBoolean(kP3AEnabled, false);

  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 100));

  EXPECT_EQ(points_requests_made, 0U);
  EXPECT_EQ(p3a_json_sent_metrics.size(), 0U);
  EXPECT_EQ(p2a_json_sent_metrics.size(), 0U);
  EXPECT_EQ(p3a_constellation_sent_messages.size(), 0U);

  current_epoch++;
  next_epoch_time += base::Days(kEpochLenDays);
  task_environment_.FastForwardBy(base::Days(kEpochLenDays) +
                                  base::Seconds(kUploadIntervalSeconds * 100));

  EXPECT_EQ(points_requests_made, 0U);
  EXPECT_EQ(p3a_constellation_sent_messages.size(), 0U);
}

}  // namespace p3a
