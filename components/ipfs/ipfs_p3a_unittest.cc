// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ipfs/ipfs_p3a.h"

#include <memory>
#include "base/test/metrics/histogram_tester.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ipfs {

class IPFSP3ATest : public testing::Test {
 public:
  IPFSP3ATest() = default;

 protected:
  void SetUp() override {
    auto* registry = pref_service_.registry();
    IpfsService::RegisterProfilePrefs(registry);
    histogram_tester_.reset(new base::HistogramTester);
  }

  PrefService* GetPrefs() { return &pref_service_; }

  std::unique_ptr<base::HistogramTester> histogram_tester_;
  TestingPrefServiceSimple pref_service_;
  content::BrowserTaskEnvironment task_environment_;
};

TEST_F(IPFSP3ATest, TestGetIPFSDetectionPromptBucket) {
  auto* prefs = GetPrefs();
  ASSERT_EQ(GetIPFSDetectionPromptBucket(prefs), 0);
  prefs->SetInteger(kIPFSInfobarCount, 1);
  ASSERT_EQ(GetIPFSDetectionPromptBucket(prefs), 1);
  prefs->SetInteger(kIPFSInfobarCount, 2);
  ASSERT_EQ(GetIPFSDetectionPromptBucket(prefs), 2);
  prefs->SetInteger(kIPFSInfobarCount, 3);
  ASSERT_EQ(GetIPFSDetectionPromptBucket(prefs), 2);
  prefs->SetInteger(kIPFSInfobarCount, 5);
  ASSERT_EQ(GetIPFSDetectionPromptBucket(prefs), 2);
  prefs->SetInteger(kIPFSInfobarCount, 6);
  ASSERT_EQ(GetIPFSDetectionPromptBucket(prefs), 3);
  prefs->SetInteger(kIPFSInfobarCount, 1337);
  ASSERT_EQ(GetIPFSDetectionPromptBucket(prefs), 3);
}

TEST_F(IPFSP3ATest, TestGetDaemonUsageBucket) {
  ASSERT_EQ(GetDaemonUsageBucket(base::Minutes(0)), 0);
  ASSERT_EQ(GetDaemonUsageBucket(base::Minutes(5)), 0);
  ASSERT_EQ(GetDaemonUsageBucket(base::Minutes(6)), 1);
  ASSERT_EQ(GetDaemonUsageBucket(base::Minutes(60)), 1);
  ASSERT_EQ(GetDaemonUsageBucket(base::Minutes(61)), 2);
  ASSERT_EQ(GetDaemonUsageBucket(base::Hours(24)), 2);
  ASSERT_EQ(GetDaemonUsageBucket(base::Hours(25)), 3);
  ASSERT_EQ(GetDaemonUsageBucket(base::Days(1337)), 3);
}

TEST_F(IPFSP3ATest, TestResolveHistogram) {
  auto* prefs = GetPrefs();

  IpfsP3A instance(nullptr, prefs);

  histogram_tester_->ExpectBucketCount(kGatewaySettingHistogramName, 0, 1);

  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(IPFSResolveMethodTypes::IPFS_GATEWAY));
  base::RunLoop().RunUntilIdle();
  histogram_tester_->ExpectBucketCount(kGatewaySettingHistogramName, 1, 1);

  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));
  base::RunLoop().RunUntilIdle();
  histogram_tester_->ExpectBucketCount(kGatewaySettingHistogramName, 2, 1);

  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(IPFSResolveMethodTypes::IPFS_DISABLED));
  base::RunLoop().RunUntilIdle();
  histogram_tester_->ExpectBucketCount(kGatewaySettingHistogramName, 3, 1);

  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));
  base::RunLoop().RunUntilIdle();
  histogram_tester_->ExpectBucketCount(kGatewaySettingHistogramName, 2, 2);
}

TEST_F(IPFSP3ATest, TestLocalNodeRetentionHistogram) {
  auto* prefs = GetPrefs();
  IpfsP3A instance(nullptr, prefs);

  histogram_tester_->ExpectBucketCount(kLocalNodeRetentionHistogramName, 0, 1);
  ASSERT_FALSE(prefs->GetBoolean(kIPFSLocalNodeUsed));

  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(IPFSResolveMethodTypes::IPFS_GATEWAY));
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(prefs->GetBoolean(kIPFSLocalNodeUsed));
  histogram_tester_->ExpectBucketCount(kLocalNodeRetentionHistogramName, 0, 2);

  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(prefs->GetBoolean(kIPFSLocalNodeUsed));
  histogram_tester_->ExpectBucketCount(kLocalNodeRetentionHistogramName, 1, 1);

  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(IPFSResolveMethodTypes::IPFS_ASK));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(prefs->GetBoolean(kIPFSLocalNodeUsed));
  histogram_tester_->ExpectBucketCount(kLocalNodeRetentionHistogramName, 1, 2);

  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(IPFSResolveMethodTypes::IPFS_GATEWAY));
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(prefs->GetBoolean(kIPFSLocalNodeUsed));
  histogram_tester_->ExpectBucketCount(kLocalNodeRetentionHistogramName, 2, 1);
}

}  // namespace ipfs
