/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/tld_ephemeral_lifetime.h"

#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ephemeral_storage {

class TLDEphemeralLifetimeTest : public testing::Test {
 private:
  content::BrowserTaskEnvironment task_environment_;

 protected:
  TestingProfile profile_;
};

TEST_F(TLDEphemeralLifetimeTest, GetOrCreateWithNullServiceReturnsNull) {
  const content::StoragePartitionConfig storage_partition_config =
      content::StoragePartitionConfig::CreateDefault(&profile_);
  const TLDEphemeralLifetimeKey key{&profile_, "a.com",
                                    storage_partition_config};

  EXPECT_FALSE(TLDEphemeralLifetime::GetOrCreate(key, nullptr));
  EXPECT_FALSE(TLDEphemeralLifetime::Get(key));
}

}  // namespace ephemeral_storage
