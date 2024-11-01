// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>

#include "base/files/scoped_temp_dir.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_testing_shard_tree_impl.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"

static_assert(BUILDFLAG(ENABLE_ORCHARD));

namespace brave_wallet {

class OrchardShardTreeUnitTest : public testing::Test {
 public:
  OrchardShardTreeUnitTest() {}

  ~OrchardShardTreeUnitTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath db_path(
        temp_dir_.GetPath().Append(FILE_PATH_LITERAL("orchard.db")));
    storage_ = base::MakeRefCounted<ZCashOrchardStorage>(path_to_database);
    shard_tree_ = OrchardTestingShardTreeImpl::Create(
        td::make_unique<OrchardShardTreeDelegateImpl>(account_id.Clone(),
                                                      storage_));
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;

  scoped_refptr<ZCashOrchardStorage> storage_;
  std::unique_ptr<OrchardTestingShardTreeImpl> shard_tree_;
};

TEST_F(OrchardShardTreeUnitTest, DiscoverNewNotes) {}

}  // namespace brave_wallet
