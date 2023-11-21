/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/extension_metrics_service.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "content/public/test/browser_task_environment.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/uninstall_reason.h"
#include "extensions/common/extension_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

namespace {
constexpr char kUBOExtensionName[] = "uBO";
constexpr char kUBOExtensionId[] = "cjpalhdlnbpafiamejdnhcphjbkeiagm";
}  // namespace

class ExtensionMetricsServiceTest : public testing::Test {
 public:
  ExtensionMetricsServiceTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  void SetUp() override {
    ubo_extension_ = extensions::ExtensionBuilder(kUBOExtensionName)
                         .SetID(kUBOExtensionId)
                         .Build();
    extension_registry_ =
        std::make_unique<extensions::ExtensionRegistry>(nullptr);
  }

  void SetUpMetrics() {
    extension_metrics_ =
        std::make_unique<ExtensionMetricsService>(extension_registry_.get());
  }

  scoped_refptr<const extensions::Extension> ubo_extension_;
  std::unique_ptr<extensions::ExtensionRegistry> extension_registry_;
  std::unique_ptr<ExtensionMetricsService> extension_metrics_;
  base::HistogramTester histogram_tester_;
  content::BrowserTaskEnvironment task_environment_;
};

TEST_F(ExtensionMetricsServiceTest, InstallAtRuntime) {
  SetUpMetrics();
  histogram_tester_.ExpectTotalCount(kAdblockExtensionsHistogramName, 0);

  task_environment_.FastForwardBy(base::Seconds(11));

  // Report once after init
  histogram_tester_.ExpectUniqueSample(kAdblockExtensionsHistogramName, 0, 1);

  // Ensure metric is recorded only once after init
  task_environment_.FastForwardBy(base::Seconds(11));
  histogram_tester_.ExpectUniqueSample(kAdblockExtensionsHistogramName, 0, 1);

  extension_registry_->AddEnabled(ubo_extension_);
  extension_registry_->TriggerOnLoaded(ubo_extension_.get());

  // Should wait at least 10 seconds for metric to update
  histogram_tester_.ExpectTotalCount(kAdblockExtensionsHistogramName, 1);
  task_environment_.FastForwardBy(base::Seconds(11));
  histogram_tester_.ExpectBucketCount(kAdblockExtensionsHistogramName, 1, 1);

  extension_registry_->RemoveEnabled(kUBOExtensionId);
  extension_registry_->TriggerOnUninstalled(
      ubo_extension_.get(),
      extensions::UninstallReason::UNINSTALL_REASON_USER_INITIATED);

  histogram_tester_.ExpectTotalCount(kAdblockExtensionsHistogramName, 2);
  task_environment_.FastForwardBy(base::Seconds(11));
  histogram_tester_.ExpectBucketCount(kAdblockExtensionsHistogramName, 0, 2);
  histogram_tester_.ExpectTotalCount(kAdblockExtensionsHistogramName, 3);
}

TEST_F(ExtensionMetricsServiceTest, LoadedAtInit) {
  extension_registry_->AddEnabled(ubo_extension_);
  SetUpMetrics();
  histogram_tester_.ExpectTotalCount(kAdblockExtensionsHistogramName, 0);

  task_environment_.FastForwardBy(base::Seconds(11));

  // Report once after init
  histogram_tester_.ExpectUniqueSample(kAdblockExtensionsHistogramName, 1, 1);

  extension_registry_->RemoveEnabled(kUBOExtensionId);
  extension_registry_->TriggerOnUninstalled(
      ubo_extension_.get(),
      extensions::UninstallReason::UNINSTALL_REASON_USER_INITIATED);

  histogram_tester_.ExpectTotalCount(kAdblockExtensionsHistogramName, 1);
  task_environment_.FastForwardBy(base::Seconds(11));
  histogram_tester_.ExpectBucketCount(kAdblockExtensionsHistogramName, 0, 1);
  histogram_tester_.ExpectTotalCount(kAdblockExtensionsHistogramName, 2);
}

}  // namespace misc_metrics
