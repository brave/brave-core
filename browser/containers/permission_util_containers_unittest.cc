// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permission_util.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/test/storage_partition_test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace containers {

class PermissionUtilContainersTest : public testing::Test {
 public:
  PermissionUtilContainersTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kContainers);
  }
  ~PermissionUtilContainersTest() override = default;

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(PermissionUtilContainersTest,
       IsPermissionBlockedInPartition_NotBlockedInContainersPartition) {
  const GURL requesting_origin("https://example.com");
  const auto containers_config =
      content::CreateStoragePartitionConfigForTesting(
          /*in_memory=*/false, kContainersStoragePartitionDomain,
          "test-container");
  content::MockRenderProcessHost containers_process(
      &profile_, containers_config, /*is_for_guests_only=*/false);

  EXPECT_FALSE(permissions::PermissionUtil::IsPermissionBlockedInPartition(
      ContentSettingsType::NOTIFICATIONS, requesting_origin,
      &containers_process));
}

TEST_F(PermissionUtilContainersTest,
       IsPermissionBlockedInPartition_BlockedInMismatchedDefaultPartition) {
  const GURL requesting_origin("https://example.com");
  const auto default_config = content::CreateStoragePartitionConfigForTesting(
      /*in_memory=*/false, "test_partition", "other");
  content::MockRenderProcessHost mismatched_process(
      &profile_, default_config, /*is_for_guests_only=*/false);

  EXPECT_TRUE(permissions::PermissionUtil::IsPermissionBlockedInPartition(
      ContentSettingsType::NOTIFICATIONS, requesting_origin,
      &mismatched_process));
}

}  // namespace containers
