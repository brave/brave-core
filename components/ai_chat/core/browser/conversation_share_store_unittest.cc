// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/conversation_share_store.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr char kShareUrl[] = "https://leo-ai.brave.app/shared/share-1#key-1";

}  // namespace

class ConversationShareStoreUnitTest : public testing::Test {
 public:
  ConversationShareStoreUnitTest() = default;
  ~ConversationShareStoreUnitTest() override = default;

  void SetUp() override {
    prefs::RegisterProfilePrefs(prefs_.registry());
    os_crypt_ = os_crypt_async::GetTestOSCryptAsyncForTesting(
        /*is_sync_for_unittests=*/true);
    CreateStore();
  }

  void CreateStore() {
    store_ = std::make_unique<ConversationShareStore>(&prefs_, os_crypt_.get());
  }

  std::vector<mojom::ConversationSharePtr> GetShares() {
    base::test::TestFuture<std::vector<mojom::ConversationSharePtr>> future;
    store_->GetShares(future.GetCallback());
    return future.Take();
  }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_;
  std::unique_ptr<ConversationShareStore> store_;
};

TEST_F(ConversationShareStoreUnitTest, StartsEmpty) {
  EXPECT_TRUE(GetShares().empty());
}

TEST_F(ConversationShareStoreUnitTest, AddAndGetShare) {
  store_->AddShare("share-1", "deletion-1", "My conversation", GURL(kShareUrl));

  std::vector<mojom::ConversationSharePtr> shares = GetShares();
  ASSERT_EQ(shares.size(), 1u);
  EXPECT_EQ(shares[0]->share_id, "share-1");
  EXPECT_EQ(shares[0]->conversation_title, "My conversation");
  EXPECT_EQ(shares[0]->created_time, base::Time::Now());
}

TEST_F(ConversationShareStoreUnitTest, IgnoresSharesThatCannotBeManaged) {
  // Without a deletion id there is nothing the management UI could do with the
  // record, so it isn't worth keeping the decryption key for.
  store_->AddShare("share-1", "", "My conversation", GURL(kShareUrl));
  store_->AddShare("", "deletion-2", "My conversation", GURL(kShareUrl));
  store_->AddShare("share-3", "deletion-3", "My conversation",
                   GURL("not-a-url"));

  EXPECT_TRUE(GetShares().empty());
}

TEST_F(ConversationShareStoreUnitTest, SharesAreMostRecentFirst) {
  store_->AddShare("share-1", "deletion-1", "First", GURL(kShareUrl));
  task_environment_.FastForwardBy(base::Hours(1));
  store_->AddShare("share-2", "deletion-2", "Second", GURL(kShareUrl));

  std::vector<mojom::ConversationSharePtr> shares = GetShares();
  ASSERT_EQ(shares.size(), 2u);
  EXPECT_EQ(shares[0]->share_id, "share-2");
  EXPECT_EQ(shares[1]->share_id, "share-1");
}

TEST_F(ConversationShareStoreUnitTest, PersistsAcrossStoreInstances) {
  store_->AddShare("share-1", "deletion-1", "My conversation", GURL(kShareUrl));

  CreateStore();

  std::vector<mojom::ConversationSharePtr> shares = GetShares();
  ASSERT_EQ(shares.size(), 1u);
  EXPECT_EQ(shares[0]->share_id, "share-1");
  EXPECT_EQ(shares[0]->conversation_title, "My conversation");
}

TEST_F(ConversationShareStoreUnitTest, StoredRecordsAreNotReadableAsPlaintext) {
  store_->AddShare("share-1", "deletion-1", "My conversation", GURL(kShareUrl));

  const std::string stored =
      prefs_.GetString(prefs::kBraveAIChatConversationShares);
  ASSERT_FALSE(stored.empty());
  std::optional<std::vector<uint8_t>> decoded = base::Base64Decode(stored);
  ASSERT_TRUE(decoded);
  const std::string decoded_string(decoded->begin(), decoded->end());
  // Neither the decryption key nor the conversation title should be legible in
  // what lands on disk.
  EXPECT_EQ(decoded_string.find("key-1"), std::string::npos);
  EXPECT_EQ(decoded_string.find("My conversation"), std::string::npos);
}

TEST_F(ConversationShareStoreUnitTest, DropsExpiredRecordsOnLoad) {
  store_->AddShare("share-1", "deletion-1", "Old", GURL(kShareUrl));
  task_environment_.FastForwardBy(base::Days(3));
  store_->AddShare("share-2", "deletion-2", "New", GURL(kShareUrl));

  // The server deletes shares after the expiry period, so the first share is
  // gone by the time the store is next created.
  task_environment_.FastForwardBy(
      base::Days(features::kAIChatConversationShareExpiryDays.Get()) -
      base::Days(2));
  CreateStore();

  std::vector<mojom::ConversationSharePtr> shares = GetShares();
  ASSERT_EQ(shares.size(), 1u);
  EXPECT_EQ(shares[0]->share_id, "share-2");
}

TEST_F(ConversationShareStoreUnitTest, DropsExpiredRecordsWhileRunning) {
  store_->AddShare("share-1", "deletion-1", "Old", GURL(kShareUrl));

  task_environment_.FastForwardBy(base::Hours(1));
  EXPECT_EQ(GetShares().size(), 1u);

  task_environment_.FastForwardBy(
      base::Days(features::kAIChatConversationShareExpiryDays.Get()));
  EXPECT_TRUE(GetShares().empty());
  EXPECT_TRUE(prefs_.GetString(prefs::kBraveAIChatConversationShares).empty());
}

TEST_F(ConversationShareStoreUnitTest, NoPendingWorkWithNothingToExpire) {
  // A perpetually armed timer would make RunLoop::Run() advance mock time
  // forever, hanging any test which merely happens to own an AIChatService.
  EXPECT_EQ(task_environment_.NextMainThreadPendingTaskDelay(),
            base::TimeDelta::Max());

  store_->AddShare("share-1", "deletion-1", "Shared", GURL(kShareUrl));
  // While a share is live, a purge is queued for exactly when it expires.
  EXPECT_EQ(task_environment_.NextMainThreadPendingTaskDelay(),
            base::Days(features::kAIChatConversationShareExpiryDays.Get()));

  // Once it has been purged there is nothing left to wait for.
  task_environment_.FastForwardBy(
      base::Days(features::kAIChatConversationShareExpiryDays.Get()));
  EXPECT_EQ(task_environment_.NextMainThreadPendingTaskDelay(),
            base::TimeDelta::Max());
}

TEST_F(ConversationShareStoreUnitTest, ExpiryPeriodIsConfigurable) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kAIChatConversationShare, {{"expiry_days", "30"}});

  store_->AddShare("share-1", "deletion-1", "Old", GURL(kShareUrl));

  task_environment_.FastForwardBy(base::Days(8));
  EXPECT_EQ(GetShares().size(), 1u);

  task_environment_.FastForwardBy(base::Days(23));
  EXPECT_TRUE(GetShares().empty());
}

TEST_F(ConversationShareStoreUnitTest, DiscardsUnreadableStoredData) {
  prefs_.SetString(prefs::kBraveAIChatConversationShares, "not-valid-base64!");

  CreateStore();

  EXPECT_TRUE(GetShares().empty());
  // The unusable value is rewritten rather than left on disk.
  EXPECT_TRUE(prefs_.GetString(prefs::kBraveAIChatConversationShares).empty());
}

}  // namespace ai_chat
