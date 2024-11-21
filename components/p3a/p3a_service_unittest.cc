// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/p3a_service.h"

#include <set>
#include <string_view>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/memory/scoped_refptr.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "brave/components/p3a/buildflags.h"
#include "brave/components/p3a/metric_names.h"
#include "brave/components/p3a/p3a_config.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/p3a/switches.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

namespace {

constexpr size_t kUploadIntervalSeconds = 60;
constexpr char kP2APrefix[] = "Brave.P2A";
constexpr char kTestCreativeMetric1[] = "creativeInstanceId.abc.views";
constexpr char kTestCreativeMetric2[] = "creativeInstanceId.abc.clicks";
constexpr char kTestExpressMetric[] = "Brave.Core.UsageDaily";
constexpr char kTestExampleMetric[] = "Brave.Core.TestMetric";

constexpr char kTestP3AJsonHost[] = "https://p3a-json.brave.com";
constexpr char kTestP2AJsonHost[] = "https://p2a-json.brave.com";
constexpr char kTestP3ACreativeHost[] = "https://p3a-creative.brave.com";

constexpr char kTestEphemeralMetric[] = "Brave.AIChat.UsageWeekly";

}  // namespace

class P3AServiceTest : public testing::Test {
 public:
  P3AServiceTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

 protected:
  void SetUp() override {
    base::Time future_mock_time;
    if (base::Time::FromString("2050-01-04", &future_mock_time)) {
      task_environment_.AdvanceClock(future_mock_time - base::Time::Now());
    }

    P3AService::RegisterPrefs(local_state_.registry(), true);

    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();

          if (request.method != net::HttpRequestHeaders::kPostMethod) {
            return;
          }

          StoreJsonMetricInMap(request, request.url);
          url_loader_factory_.AddResponse(request.url.spec(), "{}");
        }));

    config_.p3a_json_upload_url = GURL(kTestP3AJsonHost);
    config_.p2a_json_upload_url = GURL(kTestP2AJsonHost);
    config_.p3a_creative_upload_url = GURL(kTestP3ACreativeHost);
  }

  void TearDown() override { p3a_service_ = nullptr; }

  void SetUpP3AService() {
    p3a_service_ = scoped_refptr(new P3AService(
        local_state_, "release", "2049-01-01", P3AConfig(config_)));

    p3a_service_->DisableStarAttestationForTesting();
    p3a_service_->Init(shared_url_loader_factory_);
    task_environment_.RunUntilIdle();
  }

  void ResetInterceptorStores() {
    p3a_json_sent_metrics_.clear();
    p2a_json_sent_metrics_.clear();
    p3a_creative_sent_metrics_.clear();
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
                 histogram_name.starts_with("Brave.Core")) {
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
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  P3AConfig config_;
  scoped_refptr<P3AService> p3a_service_;
  TestingPrefServiceSimple local_state_;

  std::set<std::string> p3a_json_sent_metrics_;
  std::set<std::string> p2a_json_sent_metrics_;
  std::set<std::string> p3a_creative_sent_metrics_;

 private:
  std::string_view ExtractBodyFromRequest(
      const network::ResourceRequest& request) {
    return request.request_body->elements()
        ->at(0)
        .As<network::DataElementBytes>()
        .AsStringPiece();
  }

  void StoreJsonMetricInMap(const network::ResourceRequest& request,
                            const GURL& url) {
    std::string_view body = ExtractBodyFromRequest(request);
    base::Value::Dict parsed_log = base::test::ParseJsonDict(body);
    std::string* metric_name = parsed_log.FindString("metric_name");
    ASSERT_TRUE(metric_name);

    std::set<std::string>* metrics_set;
    if (url == GURL(kTestP3AJsonHost)) {
      metrics_set = &p3a_json_sent_metrics_;
    } else if (url == GURL(kTestP2AJsonHost)) {
      metrics_set = &p2a_json_sent_metrics_;
    } else if (url == GURL(kTestP3ACreativeHost)) {
      metrics_set = &p3a_creative_sent_metrics_;
    } else {
      return;
    }

    ASSERT_EQ(metrics_set->find(*metric_name), metrics_set->end());
    metrics_set->insert(*metric_name);
  }
};

TEST_F(P3AServiceTest, UpdateLogsAndSendTypical) {
  SetUpP3AService();
  std::vector<std::string> test_histograms = GetTestHistogramNames(3, 4);

  for (size_t i = 0; i < test_histograms.size(); i++) {
    base::UmaHistogramExactLinear(test_histograms[i], i + 1, 8);
    p3a_service_->OnHistogramChanged(test_histograms[i].c_str(), 0, i + 1);
    task_environment_.RunUntilIdle();
  }

  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 50));

  EXPECT_EQ(p3a_json_sent_metrics_.size(), 3U);
  EXPECT_EQ(p2a_json_sent_metrics_.size(), 4U);
  EXPECT_EQ(p3a_creative_sent_metrics_.size(), 0U);

  for (const auto& test_histogram : test_histograms) {
    if (test_histogram.rfind(kP2APrefix, 0) == 0) {
      ASSERT_NE(p2a_json_sent_metrics_.find(test_histogram),
                p2a_json_sent_metrics_.end());
    } else {
      ASSERT_NE(p3a_json_sent_metrics_.find(test_histogram),
                p3a_json_sent_metrics_.end());
    }
  }

  ResetInterceptorStores();
  task_environment_.FastForwardBy(base::Days(7) +
                                  base::Seconds(kUploadIntervalSeconds * 50));

  EXPECT_EQ(p3a_json_sent_metrics_.size(), 3U);
  EXPECT_EQ(p2a_json_sent_metrics_.size(), 4U);
  EXPECT_EQ(p3a_creative_sent_metrics_.size(), 0U);

  for (const auto& test_histogram : test_histograms) {
    if (test_histogram.rfind(kP2APrefix, 0) == 0) {
      ASSERT_NE(p2a_json_sent_metrics_.find(test_histogram),
                p2a_json_sent_metrics_.end());
    } else {
      ASSERT_NE(p3a_json_sent_metrics_.find(test_histogram),
                p3a_json_sent_metrics_.end());
    }
  }
}

TEST_F(P3AServiceTest, UpdateLogsAndSendExpress) {
  SetUpP3AService();
  std::vector<std::string> test_histograms({
      std::string(kTestExpressMetric),
      std::string(kTestCreativeMetric1),
      std::string(kTestCreativeMetric2),
  });

  p3a_service_->RegisterDynamicMetric(kTestCreativeMetric1,
                                      MetricLogType::kExpress);
  p3a_service_->RegisterDynamicMetric(kTestCreativeMetric2,
                                      MetricLogType::kExpress);

  for (size_t i = 0; i < test_histograms.size(); i++) {
    base::UmaHistogramExactLinear(test_histograms[i], i + 1, 8);
    p3a_service_->OnHistogramChanged(test_histograms[i].c_str(), 0, i + 1);
    task_environment_.RunUntilIdle();
  }

  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 100));

  EXPECT_EQ(p3a_json_sent_metrics_.size(), 1U);
  EXPECT_EQ(p2a_json_sent_metrics_.size(), 0U);
  EXPECT_EQ(p3a_creative_sent_metrics_.size(), 2U);

  ResetInterceptorStores();
  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 100));

  // Should not resend on same day
  EXPECT_EQ(p3a_json_sent_metrics_.size(), 0U);
  EXPECT_EQ(p2a_json_sent_metrics_.size(), 0U);
  EXPECT_EQ(p3a_creative_sent_metrics_.size(), 0U);

  task_environment_.FastForwardBy(base::Days(1) +
                                  base::Seconds(kUploadIntervalSeconds * 100));

  EXPECT_EQ(p3a_json_sent_metrics_.size(), 1U);
  EXPECT_EQ(p2a_json_sent_metrics_.size(), 0U);
  // Creative metrics are automatically ephemeral, should not auto resend.
  EXPECT_EQ(p3a_creative_sent_metrics_.size(), 0U);

  for (size_t i = 1; i < test_histograms.size(); i++) {
    base::UmaHistogramExactLinear(test_histograms[i], i + 1, 8);
    p3a_service_->OnHistogramChanged(test_histograms[i].c_str(), 0, i + 1);
    task_environment_.RunUntilIdle();
  }
  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 10));

  EXPECT_EQ(p3a_creative_sent_metrics_.size(), 2U);

  ResetInterceptorStores();

  p3a_service_->RemoveDynamicMetric(kTestCreativeMetric1);
  p3a_service_->RemoveDynamicMetric(kTestCreativeMetric2);

  task_environment_.FastForwardBy(base::Days(1) +
                                  base::Seconds(kUploadIntervalSeconds * 100));

  EXPECT_EQ(p3a_json_sent_metrics_.size(), 1U);
  EXPECT_EQ(p2a_json_sent_metrics_.size(), 0U);
  // Dynamic metrics should have been removed
  EXPECT_EQ(p3a_creative_sent_metrics_.size(), 0U);
}

#if !BUILDFLAG(IS_MAC)
#define MAYBE_UpdateLogsAndSendSlow UpdateLogsAndSendSlow
#else
#define MAYBE_UpdateLogsAndSendSlow DISABLED_UpdateLogsAndSendSlow
#endif

TEST_F(P3AServiceTest, MAYBE_UpdateLogsAndSendSlow) {
  // Increase upload interval to reduce test time (less tasks to execute)
  config_.average_upload_interval = base::Seconds(6000);
  SetUpP3AService();

  std::vector<std::string> test_histograms(
      {"Brave.Core.ProfileCount", std::string(kTestExampleMetric)});

  p3a_service_->RegisterDynamicMetric(kTestExampleMetric, MetricLogType::kSlow);

  for (size_t i = 0; i < test_histograms.size(); i++) {
    base::UmaHistogramExactLinear(test_histograms[i], i + 1, 8);
    p3a_service_->OnHistogramChanged(test_histograms[i].c_str(), 0, i + 1);
    task_environment_.RunUntilIdle();
  }

  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 400));

  EXPECT_EQ(p3a_json_sent_metrics_.size(), 2U);
  EXPECT_EQ(p2a_json_sent_metrics_.size(), 0U);
  EXPECT_EQ(p3a_creative_sent_metrics_.size(), 0U);

  ResetInterceptorStores();
  task_environment_.FastForwardBy(base::Days(20));

  // Should not resend on same month
  EXPECT_EQ(p3a_json_sent_metrics_.size(), 0U);
  EXPECT_EQ(p2a_json_sent_metrics_.size(), 0U);
  EXPECT_EQ(p3a_creative_sent_metrics_.size(), 0U);

  // Fast forward to the first of the next month
  task_environment_.FastForwardBy(base::Days(15) +
                                  base::Seconds(kUploadIntervalSeconds * 400));

  EXPECT_EQ(p3a_json_sent_metrics_.size(), 1U);
  EXPECT_EQ(p2a_json_sent_metrics_.size(), 0U);
  EXPECT_EQ(p3a_creative_sent_metrics_.size(), 0U);

  base::UmaHistogramExactLinear(kTestExampleMetric, 1, 8);
  p3a_service_->OnHistogramChanged(kTestExampleMetric, 0, 1);
  task_environment_.RunUntilIdle();
  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 400));

  EXPECT_EQ(p3a_json_sent_metrics_.size(), 2U);

  ResetInterceptorStores();

  p3a_service_->RemoveDynamicMetric(kTestExampleMetric);

  task_environment_.FastForwardBy(base::Days(45));

  // Dynamic metrics should have been removed
  EXPECT_EQ(p3a_json_sent_metrics_.size(), 1U);
  EXPECT_EQ(p2a_json_sent_metrics_.size(), 0U);
  EXPECT_EQ(p3a_creative_sent_metrics_.size(), 0U);
}

TEST_F(P3AServiceTest, MetricSentCallback) {
  SetUpP3AService();
  std::vector<std::string> sent_histograms;

  base::CallbackListSubscription sub =
      p3a_service_->RegisterMetricCycledCallback(base::BindLambdaForTesting(
          [&sent_histograms](const std::string& histogram_name,
                             bool is_constellation) {
            if (is_constellation) {
              return;
            }
            sent_histograms.push_back(histogram_name);
          }));

  std::vector<std::string> test_histograms = GetTestHistogramNames(3, 4);

  for (size_t i = 0; i < test_histograms.size(); i++) {
    base::UmaHistogramExactLinear(test_histograms[i], i + 1, 8);
    p3a_service_->OnHistogramChanged(test_histograms[i].c_str(), 0, i + 1);
    task_environment_.RunUntilIdle();
  }

  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 50));

  EXPECT_EQ(sent_histograms.size(), test_histograms.size());

  for (const std::string& histogram_name : sent_histograms) {
    EXPECT_NE(std::find(test_histograms.begin(), test_histograms.end(),
                        histogram_name),
              test_histograms.end());
  }
}

TEST_F(P3AServiceTest, ShouldNotSendIfDisabled) {
  SetUpP3AService();
  std::vector<std::string> test_histograms = GetTestHistogramNames(3, 3);

  for (const std::string& histogram_name : test_histograms) {
    base::UmaHistogramExactLinear(histogram_name, 5, 8);
    p3a_service_->OnHistogramChanged(histogram_name.c_str(), 0, 5);
  }

  local_state_.SetBoolean(kP3AEnabled, false);

  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 100));

  EXPECT_EQ(p3a_json_sent_metrics_.size(), 0U);
  EXPECT_EQ(p2a_json_sent_metrics_.size(), 0U);
  EXPECT_EQ(p3a_creative_sent_metrics_.size(), 0U);
}

TEST_F(P3AServiceTest, EphemeralMetricOnlySentOnce) {
  SetUpP3AService();

  base::UmaHistogramExactLinear(kTestEphemeralMetric, 1, 8);
  p3a_service_->OnHistogramChanged(kTestEphemeralMetric, 0, 8);

  EXPECT_EQ(p3a_json_sent_metrics_.size(), 0U);

  task_environment_.FastForwardBy(base::Seconds(kUploadIntervalSeconds * 10));

  EXPECT_EQ(p3a_json_sent_metrics_.size(), 1U);
  EXPECT_EQ(p2a_json_sent_metrics_.size(), 0U);
  EXPECT_EQ(p3a_creative_sent_metrics_.size(), 0U);

  ResetInterceptorStores();
  task_environment_.FastForwardBy(base::Days(7) +
                                  base::Seconds(kUploadIntervalSeconds * 10));

  EXPECT_EQ(p3a_json_sent_metrics_.size(), 0U);
  EXPECT_EQ(p2a_json_sent_metrics_.size(), 0U);
  EXPECT_EQ(p3a_creative_sent_metrics_.size(), 0U);
}

}  // namespace p3a
