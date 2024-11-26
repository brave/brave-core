// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/message_manager.h"

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "base/i18n/time_formatting.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "brave/components/p3a/features.h"
#include "brave/components/p3a/metric_log_type.h"
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
constexpr base::TimeDelta kEpochLenTimeDelta = base::Days(4);
constexpr char kTestJsonHost[] = "https://localhost:8443";
constexpr char kTestStarRandomnessHost[] = "https://localhost:9443";
constexpr char kTestStarUploadHost[] = "https://localhost:10443";
constexpr char kP2APrefix[] = "Brave.P2A";

}  // namespace

class P3AMessageManagerTest : public testing::Test,
                              public MessageManager::Delegate {
 public:
  P3AMessageManagerTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  std::optional<MetricLogType> GetDynamicMetricLogType(
      const std::string& histogram_name) const override {
    return std::optional<MetricLogType>();
  }

  void OnRotation(MetricLogType log_type, bool is_constellation) override {}

  void OnMetricCycled(const std::string& histogram_name,
                      bool is_constellation) override {}

 protected:
  void InitFeatures(bool is_constellation_enabled) {
    if (is_constellation_enabled) {
      scoped_feature_list_.InitWithFeatures(
          {features::kConstellation,
           features::kConstellationEnclaveAttestation},
          {});
    } else {
      scoped_feature_list_.InitWithFeatures(
          {}, {features::kConstellation,
               features::kConstellationEnclaveAttestation});
    }
    base::Time future_mock_time;
    if (base::Time::FromString("2050-01-04", &future_mock_time)) {
      task_environment_.AdvanceClock(future_mock_time - base::Time::Now());
    }
  }

  void SetUpManager() {
    p3a_config_.disable_star_attestation = true;
    p3a_config_.star_randomness_host = kTestStarRandomnessHost;
    p3a_config_.randomize_upload_interval = false;
    p3a_config_.average_upload_interval = base::Seconds(kUploadIntervalSeconds);
    p3a_config_.p3a_json_upload_url =
        GURL(std::string(kTestJsonHost) + "/p3a_json");
    p3a_config_.p2a_json_upload_url =
        GURL(std::string(kTestJsonHost) + "/p2a_json");
    p3a_config_.p3a_constellation_upload_host = kTestStarUploadHost;

    local_state_ = std::make_unique<TestingPrefServiceSimple>();
    P3AService::RegisterPrefs(local_state_->registry(), true);

    current_epoch_ = 0;
    next_epoch_time_ = base::Time::Now() + kEpochLenTimeDelta;

    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();

          std::string response = "{}";

          if (request.url.spec().starts_with(kTestStarRandomnessHost)) {
            MetricLogType log_type = ValidateURLAndGetMetricLogType(
                request.url, kTestStarRandomnessHost);

            if (interceptor_invalid_response_from_randomness_) {
              // next epoch time is missing!
              response = base::StrCat(
                  {"{\"currentEpoch\":", base::NumberToString(current_epoch_),
                   "}"});
            } else if (interceptor_invalid_response_from_randomness_non_json_) {
              response = "invalid response that is not json";
            } else if (request.url.spec().ends_with("/info")) {
              EXPECT_EQ(request.method, net::HttpRequestHeaders::kGetMethod);
              std::string next_epoch_time_str =
                  TimeFormatAsIso8601(next_epoch_time_);
              response = HandleInfoRequest(request, log_type, current_epoch_,
                                           next_epoch_time_str.c_str());
              info_request_made_[log_type] = true;
            } else if (request.url.spec().ends_with("/randomness")) {
              response = HandleRandomnessRequest(request, current_epoch_);
              url_loader_factory_.AddResponse(
                  request.url.spec(), response,
                  interceptor_status_code_from_randomness_);

              points_requests_made_[log_type]++;
            }
            url_loader_factory_.AddResponse(
                request.url.spec(), response,
                interceptor_status_code_from_randomness_);
            return;
          } else if (request.url == p3a_config_.p3a_json_upload_url) {
            EXPECT_EQ(request.method, net::HttpRequestHeaders::kPostMethod);
            StoreJsonMetricInMap(request, false);
          } else if (request.url == p3a_config_.p2a_json_upload_url) {
            EXPECT_EQ(request.method, net::HttpRequestHeaders::kPostMethod);
            StoreJsonMetricInMap(request, true);
          } else if (request.url.spec().starts_with(
                         std::string(kTestStarUploadHost))) {
            std::string log_type_str = request.url.path();
            EXPECT_TRUE(base::TrimString(log_type_str, "/", &log_type_str));
            std::optional<MetricLogType> log_type =
                StringToMetricLogType(log_type_str);
            EXPECT_TRUE(log_type.has_value());

            EXPECT_EQ(request.method, net::HttpRequestHeaders::kPostMethod);
            std::string message = std::string(ExtractBodyFromRequest(request));
            EXPECT_EQ(p3a_constellation_sent_messages_[*log_type].find(message),
                      p3a_constellation_sent_messages_[*log_type].end());
            p3a_constellation_sent_messages_[*log_type].insert(message);
          }
          url_loader_factory_.AddResponse(request.url.spec(), response);
        }));

    message_manager_ = std::make_unique<MessageManager>(
        *local_state_, &p3a_config_, *this, "release", "2099-01-01");

    message_manager_->Start(shared_url_loader_factory_);

    task_environment_.RunUntilIdle();
  }

  void ResetInterceptorStores() {
    p3a_json_sent_metrics_.clear();
    p2a_json_sent_metrics_.clear();
    p3a_constellation_sent_messages_.clear();
    info_request_made_.clear();
    points_requests_made_.clear();
  }

  base::TimeDelta GetJsonRotationTimeDelta(MetricLogType log_type) {
    switch (log_type) {
      case MetricLogType::kExpress:
        return base::Days(1);
      case MetricLogType::kTypical:
        return base::Days(7);
      case MetricLogType::kSlow:
        return base::Days(31);
    }
  }

  std::vector<std::string> GetTestHistogramNames(MetricLogType log_type,
                                                 size_t p3a_count,
                                                 size_t p2a_count) {
    auto histograms_begin = kCollectedExpressHistograms.cbegin();
    auto histograms_end = kCollectedExpressHistograms.cend();
    std::vector<std::string> result;
    size_t p3a_i = 0;
    size_t p2a_i = 0;
    switch (log_type) {
      case MetricLogType::kExpress:
        histograms_begin = kCollectedExpressHistograms.cbegin();
        histograms_end = kCollectedExpressHistograms.cend();
        break;
      case MetricLogType::kSlow:
        histograms_begin = kCollectedSlowHistograms.cbegin();
        histograms_end = kCollectedSlowHistograms.cend();
        break;
      case MetricLogType::kTypical:
        histograms_begin = kCollectedTypicalHistograms.cbegin();
        histograms_end = kCollectedTypicalHistograms.cend();
        break;
      default:
        NOTREACHED();
    }
    for (auto histogram_i = histograms_begin; histogram_i != histograms_end;
         histogram_i++) {
      if (histogram_i->first.rfind(kP2APrefix, 0) == 0) {
        if (p2a_i < p2a_count) {
          result.push_back(std::string(histogram_i->first));
          p2a_i++;
        }
      } else if (p3a_i < p3a_count &&
                 (histogram_i->first.starts_with("Brave.Core") ||
                  log_type == MetricLogType::kExpress)) {
        result.push_back(std::string(histogram_i->first));
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
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  P3AConfig p3a_config_;
  std::unique_ptr<MessageManager> message_manager_;
  std::unique_ptr<TestingPrefServiceSimple> local_state_;

  base::flat_map<std::string, size_t> p3a_json_sent_metrics_;
  base::flat_map<std::string, size_t> p2a_json_sent_metrics_;

  base::flat_map<MetricLogType, base::flat_set<std::string>>
      p3a_constellation_sent_messages_;

  bool interceptor_invalid_response_from_randomness_ = false;
  bool interceptor_invalid_response_from_randomness_non_json_ = false;
  net::HttpStatusCode interceptor_status_code_from_randomness_ = net::HTTP_OK;
  bool ignore_json_duplicates_ = false;

  base::flat_map<MetricLogType, bool> info_request_made_;
  base::flat_map<MetricLogType, size_t> points_requests_made_;

  uint8_t current_epoch_ = kInitialEpoch;
  base::Time next_epoch_time_;

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
    std::optional<int> metric_value = parsed_log.FindInt("metric_value");
    ASSERT_TRUE(metric_value);

    if (is_p2a) {
      if (!ignore_json_duplicates_) {
        EXPECT_EQ(p2a_json_sent_metrics_.find(*metric_name),
                  p2a_json_sent_metrics_.end());
      }
      p2a_json_sent_metrics_[*metric_name] = *metric_value;
    } else {
      if (!ignore_json_duplicates_) {
        EXPECT_EQ(p3a_json_sent_metrics_.find(*metric_name),
                  p3a_json_sent_metrics_.end());
      }
      p3a_json_sent_metrics_[*metric_name] = *metric_value;
    }
  }
};

TEST_F(P3AMessageManagerTest, UpdateLogsAndSendJson) {
  InitFeatures(true);
  for (MetricLogType log_type : kAllMetricLogTypes) {
    size_t p2a_count = log_type == MetricLogType::kTypical ? 4 : 0;
    SetUpManager();
    ResetInterceptorStores();
    std::vector<std::string> test_histograms =
        GetTestHistogramNames(log_type, 3, p2a_count);

    for (size_t i = 0; i < test_histograms.size(); i++) {
      message_manager_->UpdateMetricValue(test_histograms[i], i + 1);
    }

    task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 50));

    EXPECT_EQ(p3a_json_sent_metrics_.size(), 3U);
    EXPECT_EQ(p2a_json_sent_metrics_.size(), p2a_count);

    for (size_t i = 0; i < test_histograms.size(); i++) {
      if (test_histograms[i].rfind(kP2APrefix, 0) == 0) {
        ASSERT_EQ(p2a_json_sent_metrics_.find(test_histograms[i])->second,
                  i + 1);
      } else {
        ASSERT_EQ(p3a_json_sent_metrics_.find(test_histograms[i])->second,
                  i + 1);
      }
    }

    if (log_type == MetricLogType::kExpress) {
      // Most express metrics are ephemeral, so they won't be sent again.
      // No need to run tests below.
      continue;
    }

    ResetInterceptorStores();
    task_environment_.FastForwardBy(GetJsonRotationTimeDelta(log_type) +
                                    base::Seconds(kUploadIntervalSeconds * 50));

    EXPECT_EQ(p3a_json_sent_metrics_.size(), 3U);
    EXPECT_EQ(p2a_json_sent_metrics_.size(), p2a_count);
    for (size_t i = 0; i < test_histograms.size(); i++) {
      if (test_histograms[i].rfind(kP2APrefix, 0) == 0) {
        ASSERT_EQ(p2a_json_sent_metrics_.find(test_histograms[i])->second,
                  i + 1);
      } else {
        ASSERT_EQ(p3a_json_sent_metrics_.find(test_histograms[i])->second,
                  i + 1);
      }
    }
  }
}

TEST_F(P3AMessageManagerTest, UpdateLogsAndDontSendConstellation) {
  ignore_json_duplicates_ = true;
  // don't perform any constellation activity if feature is disabled
  InitFeatures(false);
  SetUpManager();
  for (MetricLogType log_type : kAllMetricLogTypes) {
    ResetInterceptorStores();
    ASSERT_FALSE(info_request_made_[log_type]);

    std::vector<std::string> test_histograms =
        GetTestHistogramNames(log_type, 7, 0);

    for (size_t i = 0; i < test_histograms.size(); i++) {
      message_manager_->UpdateMetricValue(test_histograms[i], i + 1);
    }

    task_environment_.FastForwardBy(
        base::Seconds(kUploadIntervalSeconds * 100));

    EXPECT_EQ(points_requests_made_[log_type], 0U);
    EXPECT_EQ(p3a_constellation_sent_messages_[log_type].size(), 0U);

    ResetInterceptorStores();
    current_epoch_++;
    next_epoch_time_ += kEpochLenTimeDelta;
    task_environment_.FastForwardBy(
        kEpochLenTimeDelta + base::Seconds(kUploadIntervalSeconds * 100));

    ASSERT_FALSE(info_request_made_[log_type]);
    EXPECT_EQ(points_requests_made_[log_type], 0U);
    EXPECT_EQ(p3a_constellation_sent_messages_[log_type].size(), 0U);
  }
}

TEST_F(P3AMessageManagerTest, UpdateLogsAndSendConstellation) {
  ignore_json_duplicates_ = true;
  InitFeatures(true);
  for (MetricLogType log_type : kAllMetricLogTypes) {
    ResetInterceptorStores();
    SetUpManager();
    ASSERT_TRUE(info_request_made_[log_type]);

    std::vector<std::string> test_histograms =
        GetTestHistogramNames(log_type, 3, 0);

    for (size_t i = 0; i < test_histograms.size(); i++) {
      message_manager_->UpdateMetricValue(test_histograms[i], i + 1);
    }

    task_environment_.FastForwardBy(
        base::Seconds(kUploadIntervalSeconds * 100));

    EXPECT_EQ(points_requests_made_[log_type], 3U);
    EXPECT_EQ(p3a_constellation_sent_messages_[log_type].size(), 3U);

    ResetInterceptorStores();
    current_epoch_++;
    next_epoch_time_ += kEpochLenTimeDelta;
    task_environment_.FastForwardBy(
        kEpochLenTimeDelta + base::Seconds(kUploadIntervalSeconds * 100));

    ASSERT_TRUE(info_request_made_[log_type]);
    if (log_type != MetricLogType::kExpress) {
      // We can only check non-express metrics, since there are very little
      // non-ephemeral metrics for the express cadence.
      EXPECT_EQ(points_requests_made_[log_type], 3U);
      EXPECT_EQ(p3a_constellation_sent_messages_[log_type].size(), 3U);
    }

    ResetInterceptorStores();
    current_epoch_++;
    next_epoch_time_ += kEpochLenTimeDelta;
    task_environment_.FastForwardBy(
        kEpochLenTimeDelta + base::Seconds(kUploadIntervalSeconds * 100));

    ASSERT_TRUE(info_request_made_[log_type]);
    if (log_type != MetricLogType::kExpress) {
      // We can only check non-express metrics, since there are very little
      // non-ephemeral metrics for the express cadence.
      EXPECT_EQ(points_requests_made_[log_type], 3U);
      EXPECT_EQ(p3a_constellation_sent_messages_[log_type].size(), 3U);
    }
  }
}

TEST_F(P3AMessageManagerTest, UpdateLogsAndSendConstellationInvalidResponse) {
  ignore_json_duplicates_ = true;
  InitFeatures(true);
  for (MetricLogType log_type : kAllMetricLogTypes) {
    ResetInterceptorStores();
    SetUpManager();
    ASSERT_TRUE(info_request_made_[log_type]);

    std::vector<std::string> test_histograms =
        GetTestHistogramNames(log_type, 3, 0);

    for (size_t i = 0; i < test_histograms.size(); i++) {
      message_manager_->UpdateMetricValue(test_histograms[i], i + 1);
    }

    task_environment_.FastForwardBy(
        base::Seconds(kUploadIntervalSeconds * 100));
    EXPECT_EQ(points_requests_made_[log_type], 3U);
    ResetInterceptorStores();

    // server will return invalid response body that is json, but has missing
    // fields
    interceptor_invalid_response_from_randomness_ = true;

    if (log_type != MetricLogType::kSlow) {
      // skip ahead to next epoch, only if log type is not slow
      // (because the max epoch rotation for slow is only 2 epochs)
      current_epoch_++;
    }
    next_epoch_time_ += kEpochLenTimeDelta;
    task_environment_.FastForwardBy(kEpochLenTimeDelta);

    EXPECT_EQ(points_requests_made_[log_type], 0U);
    // We are at the beginning of the new epoch. measurements from previous
    // epoch should not be sent since we are unable to get the current epoch
    // from the server.
    EXPECT_EQ(p3a_constellation_sent_messages_[log_type].size(), 0U);

    // server will return response body that is not json
    interceptor_invalid_response_from_randomness_ = false;
    interceptor_invalid_response_from_randomness_non_json_ = true;

    if (log_type != MetricLogType::kSlow) {
      // skip ahead to next epoch, only if log type is not slow
      // (because the max epoch rotation for slow is only 2 epochs)
      current_epoch_++;
    }
    next_epoch_time_ += kEpochLenTimeDelta;
    task_environment_.FastForwardBy(kEpochLenTimeDelta);

    EXPECT_EQ(points_requests_made_[log_type], 0U);
    // no new measurements should have been recorded in the previous epoch.
    EXPECT_EQ(p3a_constellation_sent_messages_[log_type].size(), 0U);

    // restore randomness server functionality
    interceptor_invalid_response_from_randomness_non_json_ = false;

    current_epoch_++;
    next_epoch_time_ += kEpochLenTimeDelta;
    task_environment_.FastForwardBy(kEpochLenTimeDelta);

    // randomness server is now providing correct response . no new measurements
    // should have been recorded in the previous epoch due to previous
    // unavailability. randomness points should be requested for the current
    // epoch. messages from the first epoch should be sent.
    ASSERT_TRUE(info_request_made_[log_type]);
    if (log_type != MetricLogType::kExpress) {
      // We can only check non-express metrics, since there are very little
      // non-ephemeral metrics for the express cadence.
      EXPECT_EQ(points_requests_made_[log_type], 3U);
      EXPECT_EQ(p3a_constellation_sent_messages_[log_type].size(), 3U);
    }
  }
}

TEST_F(P3AMessageManagerTest,
       UpdateLogsAndSendConstellationInvalidClientRequest) {
  ignore_json_duplicates_ = true;
  InitFeatures(true);
  for (MetricLogType log_type : kAllMetricLogTypes) {
    ResetInterceptorStores();
    SetUpManager();
    ASSERT_TRUE(info_request_made_[log_type]);

    std::vector<std::string> test_histograms =
        GetTestHistogramNames(log_type, 3, 0);

    for (size_t i = 0; i < test_histograms.size(); i++) {
      message_manager_->UpdateMetricValue(test_histograms[i], i + 1);
    }

    task_environment_.FastForwardBy(
        base::Seconds(kUploadIntervalSeconds * 100));
    ResetInterceptorStores();

    // server will return HTTP 500 to indicate an invalid client request.
    interceptor_status_code_from_randomness_ = net::HTTP_BAD_REQUEST;

    // skip ahead to next epoch
    if (log_type != MetricLogType::kSlow) {
      // skip ahead to next epoch, only if log type is not slow
      // (because the max epoch rotation for slow is only 2 epochs)
      current_epoch_++;
    }
    next_epoch_time_ += kEpochLenTimeDelta;
    task_environment_.FastForwardBy(kEpochLenTimeDelta);

    // We are at the beginning of the new epoch. measurements from previous
    // epoch should not be sent since we are unable to get the current epoch
    // from the server.
    EXPECT_EQ(p3a_constellation_sent_messages_[log_type].size(), 0U);

    // restore randomness server functionality
    interceptor_status_code_from_randomness_ = net::HTTP_OK;

    current_epoch_++;
    next_epoch_time_ += kEpochLenTimeDelta;
    task_environment_.FastForwardBy(kEpochLenTimeDelta);

    // randomness server is now accepting client request. no new measurements
    // should have been recorded in the previous epoch due to previous
    // unavailability. randomness points should be requested for the current
    // epoch. messages from the first epoch should be sent.
    ASSERT_TRUE(info_request_made_[log_type]);
    if (log_type != MetricLogType::kExpress) {
      // We can only check non-express metrics, since there are very little
      // non-ephemeral metrics for the express cadence.
      EXPECT_EQ(points_requests_made_[log_type], 3U);
      EXPECT_EQ(p3a_constellation_sent_messages_[log_type].size(), 3U);
    }
  }
}

TEST_F(P3AMessageManagerTest, UpdateLogsAndSendConstellationUnavailable) {
  ignore_json_duplicates_ = true;
  InitFeatures(true);
  for (MetricLogType log_type : kAllMetricLogTypes) {
    interceptor_status_code_from_randomness_ = net::HTTP_OK;
    ResetInterceptorStores();
    SetUpManager();
    ASSERT_TRUE(info_request_made_[log_type]);

    std::vector<std::string> test_histograms =
        GetTestHistogramNames(log_type, 3, 0);

    for (size_t i = 0; i < test_histograms.size(); i++) {
      message_manager_->UpdateMetricValue(test_histograms[i], i + 1);
    }

    task_environment_.FastForwardBy(
        base::Seconds(kUploadIntervalSeconds * 100));
    ResetInterceptorStores();

    // server will return HTTP 500 to indicate unavailability.
    interceptor_status_code_from_randomness_ = net::HTTP_INTERNAL_SERVER_ERROR;

    if (log_type != MetricLogType::kSlow) {
      // skip ahead to next epoch, only if log type is not slow
      // (because the max epoch rotation for slow is only 2 epochs)
      current_epoch_++;
    }
    next_epoch_time_ += kEpochLenTimeDelta;
    task_environment_.FastForwardBy(kEpochLenTimeDelta);

    // We are at the beginning of the new epoch. measurements from previous
    // epoch should not be sent since we are unable to get the current epoch
    // from the server.
    EXPECT_EQ(p3a_constellation_sent_messages_[log_type].size(), 0U);

    // restore randomness server functionality
    interceptor_status_code_from_randomness_ = net::HTTP_OK;

    ResetInterceptorStores();
    current_epoch_++;
    next_epoch_time_ += kEpochLenTimeDelta;
    task_environment_.FastForwardBy(kEpochLenTimeDelta);

    // randomness server is now available. no new measurements should have been
    // recorded in the previous epoch due to previous unavailability.
    // randomness points should be requested for the current epoch.
    // messages from the first epoch should be sent.
    ASSERT_TRUE(info_request_made_[log_type]);
    if (log_type != MetricLogType::kExpress) {
      // We can only check non-express metrics, since there are very little
      // non-ephemeral metrics for the express cadence.
      EXPECT_EQ(points_requests_made_[log_type], 3U);
      EXPECT_EQ(p3a_constellation_sent_messages_[log_type].size(), 3U);
    }
  }
}

TEST_F(P3AMessageManagerTest, DoesNotSendRemovedMetricValue) {
  InitFeatures(true);
  SetUpManager();
  for (MetricLogType log_type : kAllMetricLogTypes) {
    std::vector<std::string> test_histograms =
        GetTestHistogramNames(log_type, 3, 3);

    for (const std::string& histogram_name : test_histograms) {
      message_manager_->UpdateMetricValue(histogram_name, 5);
    }

    for (const std::string& histogram_name : test_histograms) {
      message_manager_->RemoveMetricValue(histogram_name);
    }

    task_environment_.FastForwardBy(
        base::Seconds(kUploadIntervalSeconds * 100));

    EXPECT_EQ(points_requests_made_[log_type], 0U);
    EXPECT_EQ(p3a_json_sent_metrics_.size(), 0U);
    EXPECT_EQ(p2a_json_sent_metrics_.size(), 0U);

    current_epoch_++;
    next_epoch_time_ += kEpochLenTimeDelta;
    task_environment_.FastForwardBy(
        kEpochLenTimeDelta + base::Seconds(kUploadIntervalSeconds * 100));

    EXPECT_EQ(points_requests_made_[log_type], 0U);
    EXPECT_EQ(p3a_constellation_sent_messages_[log_type].size(), 0U);
  }
}

TEST_F(P3AMessageManagerTest, ShouldNotSendIfDisabled) {
  InitFeatures(true);
  SetUpManager();
  for (MetricLogType log_type : kAllMetricLogTypes) {
    std::vector<std::string> test_histograms =
        GetTestHistogramNames(log_type, 3, 3);

    for (const std::string& histogram_name : test_histograms) {
      message_manager_->UpdateMetricValue(histogram_name, 5);
    }

    local_state_->SetBoolean(kP3AEnabled, false);
    message_manager_->Stop();

    task_environment_.FastForwardBy(
        base::Seconds(kUploadIntervalSeconds * 100));

    EXPECT_EQ(points_requests_made_[log_type], 0U);
    EXPECT_EQ(p3a_json_sent_metrics_.size(), 0U);
    EXPECT_EQ(p2a_json_sent_metrics_.size(), 0U);
    EXPECT_EQ(p3a_constellation_sent_messages_[log_type].size(), 0U);

    current_epoch_++;
    next_epoch_time_ += kEpochLenTimeDelta;
    task_environment_.FastForwardBy(
        kEpochLenTimeDelta + base::Seconds(kUploadIntervalSeconds * 100));

    EXPECT_EQ(points_requests_made_[log_type], 0U);
    EXPECT_EQ(p3a_constellation_sent_messages_[log_type].size(), 0U);
  }
}

TEST_F(P3AMessageManagerTest, ShouldNotSendIfStopped) {
  InitFeatures(true);
  SetUpManager();

  message_manager_->Stop();

  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 100));

  EXPECT_TRUE(points_requests_made_.empty());
  EXPECT_TRUE(p3a_json_sent_metrics_.empty());
  EXPECT_TRUE(p2a_json_sent_metrics_.empty());
  EXPECT_TRUE(p3a_constellation_sent_messages_.empty());
}

}  // namespace p3a
