/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/extension_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "brave/browser/ui/webui/settings/brave_extensions_manifest_v2_installer.h"
#include "content/public/test/browser_task_environment.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/uninstall_reason.h"
#include "extensions/common/extension_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

namespace {
constexpr char kUBOExtensionName[] = "uBO";
constexpr char kUBOExtensionId[] = "cjpalhdlnbpafiamejdnhcphjbkeiagm";
constexpr char kManifestV2ExtensionName[] = "ManifestV2Extension";
constexpr char kManifestV2ExtensionId[] = "manifestv2extensionid";
constexpr char kManifestV3ExtensionName[] = "ManifestV3Extension";
constexpr char kManifestV3ExtensionId[] = "manifestv3extensionid";
constexpr char kNoScriptExtensionName[] = "NoScript";
}  // namespace

class ExtensionMetricsTest : public testing::Test {
 public:
  ExtensionMetricsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  void SetUp() override {
    ubo_extension_ = extensions::ExtensionBuilder(kUBOExtensionName)
                         .SetID(kUBOExtensionId)
                         .Build();
    manifest_v2_extension_ =
        extensions::ExtensionBuilder(kManifestV2ExtensionName)
            .SetID(kManifestV2ExtensionId)
            .SetManifestVersion(2)
            .Build();
    manifest_v3_extension_ =
        extensions::ExtensionBuilder(kManifestV3ExtensionName)
            .SetID(kManifestV3ExtensionId)
            .SetManifestVersion(3)
            .Build();
    noscript_extension_ = extensions::ExtensionBuilder(kNoScriptExtensionName)
                              .SetID(extensions_mv2::kNoScriptId)
                              .SetManifestVersion(2)
                              .Build();
    extension_registry_ =
        std::make_unique<extensions::ExtensionRegistry>(nullptr);
  }

  void SetUpMetrics() {
    extension_metrics_ =
        std::make_unique<ExtensionMetrics>(extension_registry_.get());
  }

  scoped_refptr<const extensions::Extension> ubo_extension_;
  scoped_refptr<const extensions::Extension> manifest_v2_extension_;
  scoped_refptr<const extensions::Extension> manifest_v3_extension_;
  scoped_refptr<const extensions::Extension> noscript_extension_;
  std::unique_ptr<extensions::ExtensionRegistry> extension_registry_;
  std::unique_ptr<ExtensionMetrics> extension_metrics_;
  base::HistogramTester histogram_tester_;
  content::BrowserTaskEnvironment task_environment_;
};

TEST_F(ExtensionMetricsTest, AdBlockInstallAtRuntime) {
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

TEST_F(ExtensionMetricsTest, AdBlockLoadedAtInit) {
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

TEST_F(ExtensionMetricsTest, ManifestV2InstallAtRuntime) {
  extension_registry_->AddEnabled(manifest_v3_extension_);

  SetUpMetrics();
  histogram_tester_.ExpectTotalCount(kManifestV2ExtensionsHistogramName, 0);

  task_environment_.FastForwardBy(base::Seconds(11));

  // Report once after init
  histogram_tester_.ExpectUniqueSample(kManifestV2ExtensionsHistogramName, 0,
                                       1);

  // Ensure metric is recorded only once after init
  task_environment_.FastForwardBy(base::Seconds(11));
  histogram_tester_.ExpectUniqueSample(kManifestV2ExtensionsHistogramName, 0,
                                       1);

  extension_registry_->AddEnabled(manifest_v2_extension_);
  extension_registry_->TriggerOnLoaded(manifest_v2_extension_.get());

  // Should wait at least 10 seconds for metric to update
  histogram_tester_.ExpectTotalCount(kManifestV2ExtensionsHistogramName, 1);
  task_environment_.FastForwardBy(base::Seconds(11));
  histogram_tester_.ExpectBucketCount(kManifestV2ExtensionsHistogramName, 1, 1);

  extension_registry_->RemoveEnabled(kManifestV2ExtensionId);
  extension_registry_->TriggerOnUninstalled(
      manifest_v2_extension_.get(),
      extensions::UninstallReason::UNINSTALL_REASON_USER_INITIATED);

  histogram_tester_.ExpectTotalCount(kManifestV2ExtensionsHistogramName, 2);
  task_environment_.FastForwardBy(base::Seconds(11));
  histogram_tester_.ExpectBucketCount(kManifestV2ExtensionsHistogramName, 0, 2);
  histogram_tester_.ExpectTotalCount(kManifestV2ExtensionsHistogramName, 3);
}

TEST_F(ExtensionMetricsTest, ManifestV2LoadedAtInit) {
  extension_registry_->AddEnabled(manifest_v3_extension_);
  extension_registry_->AddEnabled(manifest_v2_extension_);

  SetUpMetrics();
  histogram_tester_.ExpectTotalCount(kManifestV2ExtensionsHistogramName, 0);

  task_environment_.FastForwardBy(base::Seconds(11));

  // Report once after init
  histogram_tester_.ExpectUniqueSample(kManifestV2ExtensionsHistogramName, 1,
                                       1);

  extension_registry_->RemoveEnabled(kManifestV2ExtensionId);
  extension_registry_->TriggerOnUninstalled(
      manifest_v2_extension_.get(),
      extensions::UninstallReason::UNINSTALL_REASON_USER_INITIATED);

  histogram_tester_.ExpectTotalCount(kManifestV2ExtensionsHistogramName, 1);
  task_environment_.FastForwardBy(base::Seconds(11));
  histogram_tester_.ExpectBucketCount(kManifestV2ExtensionsHistogramName, 0, 1);
  histogram_tester_.ExpectTotalCount(kManifestV2ExtensionsHistogramName, 2);
}

TEST_F(ExtensionMetricsTest, SelectManifestV2LoadedAtInit) {
  // Start with a preconfigured extension loaded
  extension_registry_->AddEnabled(noscript_extension_);

  SetUpMetrics();
  histogram_tester_.ExpectTotalCount(kSelectManifestV2ExtensionsHistogramName,
                                     0);

  task_environment_.FastForwardBy(base::Seconds(11));

  // Report once after init
  histogram_tester_.ExpectUniqueSample(kSelectManifestV2ExtensionsHistogramName,
                                       1, 1);

  extension_registry_->RemoveEnabled(extensions_mv2::kNoScriptId);
  extension_registry_->TriggerOnUninstalled(
      noscript_extension_.get(),
      extensions::UninstallReason::UNINSTALL_REASON_USER_INITIATED);

  histogram_tester_.ExpectTotalCount(kSelectManifestV2ExtensionsHistogramName,
                                     1);
  task_environment_.FastForwardBy(base::Seconds(11));
  histogram_tester_.ExpectBucketCount(kSelectManifestV2ExtensionsHistogramName,
                                      0, 1);
  histogram_tester_.ExpectTotalCount(kSelectManifestV2ExtensionsHistogramName,
                                     2);
}

}  // namespace misc_metrics
