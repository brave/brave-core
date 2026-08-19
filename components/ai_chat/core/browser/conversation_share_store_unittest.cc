// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/conversation_share_store.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/proto/store.pb.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/os_crypt/async/common/test_encryptor.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr char kShareUrl[] = "https://leo-ai.brave.app/shared/share-1#key-1";

// Vends the same encryptor every time, so that a store re-created over the same
// prefs can read what an earlier one wrote, and tests can control what the
// encryptor is able to do in between.
class FakeOSCryptAsync : public os_crypt_async::OSCryptAsync {
 public:
  FakeOSCryptAsync()
      : OSCryptAsync({}),
        encryptor_(os_crypt_async::GetTestEncryptorForTesting()) {}

  void GetInstance(InitCallback callback) override {
    std::move(callback).Run(encryptor_);
  }

  os_crypt_async::TestEncryptor& encryptor() { return *encryptor_; }

 private:
  scoped_refptr<os_crypt_async::TestEncryptor> encryptor_;
};

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

  std::optional<std::string> GetDeletionId(const std::string& share_id) {
    base::test::TestFuture<std::optional<std::string>> future;
    store_->GetDeletionId(share_id, future.GetCallback());
    return future.Take();
  }

  std::optional<GURL> GetShareUrl(const std::string& share_id) {
    base::test::TestFuture<std::optional<GURL>> future;
    store_->GetShareUrl(share_id, future.GetCallback());
    return future.Take();
  }

  // Writes |proto| to the pref the way the store would, so that stored values
  // the store itself would never write can be arranged.
  void StoreProto(os_crypt_async::Encryptor& encryptor,
                  const store::ConversationSharesProto& proto) {
    std::optional<std::vector<uint8_t>> ciphertext =
        encryptor.EncryptString(proto.SerializeAsString());
    ASSERT_TRUE(ciphertext);
    prefs_.SetString(prefs::kBraveAIChatConversationShares,
                     base::Base64Encode(*ciphertext));
  }

  // What is stored in the pref, so that fields the public API doesn't report
  // back can be checked. Empty if it can't be read.
  store::ConversationSharesProto ReadStoredProto(
      os_crypt_async::Encryptor& encryptor) {
    store::ConversationSharesProto proto;
    std::optional<std::vector<uint8_t>> ciphertext = base::Base64Decode(
        prefs_.GetString(prefs::kBraveAIChatConversationShares));
    if (!ciphertext) {
      return proto;
    }
    std::optional<std::string> serialized = encryptor.DecryptData(*ciphertext);
    if (serialized) {
      proto.ParseFromString(*serialized);
    }
    return proto;
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
  store_->AddShare("share-1", "deletion-1", "conversation-share-1",
                   "My conversation", GURL(kShareUrl));

  std::vector<mojom::ConversationSharePtr> shares = GetShares();
  ASSERT_EQ(shares.size(), 1u);
  EXPECT_EQ(shares[0]->share_id, "share-1");
  EXPECT_EQ(shares[0]->conversation_uuid, "conversation-share-1");
  EXPECT_EQ(shares[0]->conversation_title, "My conversation");
  EXPECT_EQ(shares[0]->created_time, base::Time::Now());

  // The deletion id and the link (which contains the decryption key) are kept
  // for acting on the share, but aren't part of what the UI displays.
  EXPECT_EQ(GetDeletionId("share-1"), "deletion-1");
  EXPECT_EQ(GetShareUrl("share-1"), GURL(kShareUrl));
}

TEST_F(ConversationShareStoreUnitTest, RecordsEveryFieldOfAShare) {
  FakeOSCryptAsync os_crypt;
  store_ = std::make_unique<ConversationShareStore>(&prefs_, &os_crypt);
  store_->AddShare("share-1", "deletion-1", "conversation-share-1",
                   "My conversation", GURL(kShareUrl));

  // The link and the deletion id are deliberately never reported to the UI, so
  // check what was stored: without either, a share can't later be re-copied or
  // deleted.
  store::ConversationSharesProto stored = ReadStoredProto(os_crypt.encryptor());
  ASSERT_EQ(stored.shares().size(), 1);
  const store::ConversationShareProto& share = stored.shares(0);
  EXPECT_EQ(share.share_id(), "share-1");
  EXPECT_EQ(share.deletion_id(), "deletion-1");
  EXPECT_EQ(share.conversation_uuid(), "conversation-share-1");
  EXPECT_EQ(share.conversation_title(), "My conversation");
  EXPECT_EQ(share.url(), kShareUrl);
  EXPECT_EQ(base::Time::FromDeltaSinceWindowsEpoch(
                base::Microseconds(share.created_time_windows_epoch_micros())),
            base::Time::Now());
}

TEST_F(ConversationShareStoreUnitTest, DiscardsStoredRecordsMissingFields) {
  FakeOSCryptAsync os_crypt;
  const int64_t now =
      base::Time::Now().ToDeltaSinceWindowsEpoch().InMicroseconds();
  store::ConversationSharesProto proto;
  // No title, which is legitimate - conversations can be untitled.
  store::ConversationShareProto* usable = proto.add_shares();
  usable->set_share_id("share-1");
  usable->set_deletion_id("deletion-1");
  usable->set_conversation_uuid("conversation-share-1");
  usable->set_url(kShareUrl);
  usable->set_created_time_windows_epoch_micros(now);
  // No deletion id, so this one could never be removed from the server.
  store::ConversationShareProto* no_deletion_id = proto.add_shares();
  no_deletion_id->set_share_id("share-2");
  no_deletion_id->set_conversation_uuid("conversation-share-2");
  no_deletion_id->set_url(kShareUrl);
  no_deletion_id->set_created_time_windows_epoch_micros(now);
  // An empty share id is no more usable than a missing one - there would be
  // nothing to ask the server to delete.
  store::ConversationShareProto* empty_share_id = proto.add_shares();
  empty_share_id->set_share_id("");
  empty_share_id->set_deletion_id("deletion-3");
  empty_share_id->set_conversation_uuid("conversation-share-3");
  empty_share_id->set_url(kShareUrl);
  empty_share_id->set_created_time_windows_epoch_micros(now);
  ASSERT_NO_FATAL_FAILURE(StoreProto(os_crypt.encryptor(), proto));

  store_ = std::make_unique<ConversationShareStore>(&prefs_, &os_crypt);

  std::vector<mojom::ConversationSharePtr> shares = GetShares();
  ASSERT_EQ(shares.size(), 1u);
  EXPECT_EQ(shares[0]->share_id, "share-1");
  EXPECT_TRUE(shares[0]->conversation_title.empty());
  // The record which can't be acted on is dropped from the pref, not merely
  // left out of what is reported.
  store::ConversationSharesProto stored = ReadStoredProto(os_crypt.encryptor());
  ASSERT_EQ(stored.shares().size(), 1);
  EXPECT_EQ(stored.shares(0).share_id(), "share-1");
}

TEST_F(ConversationShareStoreUnitTest, IgnoresSharesThatCannotBeManaged) {
  // Without a deletion id there is nothing the management UI could do with the
  // record, so it isn't worth keeping the decryption key for.
  store_->AddShare("share-1", "", "conversation-share-1", "My conversation",
                   GURL(kShareUrl));
  store_->AddShare("", "deletion-2", "", "My conversation", GURL(kShareUrl));
  store_->AddShare("share-3", "deletion-3", "conversation-share-3",
                   "My conversation", GURL("not-a-url"));

  EXPECT_TRUE(GetShares().empty());
}

TEST_F(ConversationShareStoreUnitTest, SharesAreMostRecentFirst) {
  store_->AddShare("share-1", "deletion-1", "conversation-share-1", "First",
                   GURL(kShareUrl));
  task_environment_.FastForwardBy(base::Hours(1));
  store_->AddShare("share-2", "deletion-2", "conversation-share-2", "Second",
                   GURL(kShareUrl));

  std::vector<mojom::ConversationSharePtr> shares = GetShares();
  ASSERT_EQ(shares.size(), 2u);
  EXPECT_EQ(shares[0]->share_id, "share-2");
  EXPECT_EQ(shares[1]->share_id, "share-1");
}

TEST_F(ConversationShareStoreUnitTest, GetShareUrlForUnknownShare) {
  EXPECT_FALSE(GetShareUrl("never-shared").has_value());
}

TEST_F(ConversationShareStoreUnitTest, RemoveShare) {
  store_->AddShare("share-1", "deletion-1", "conversation-share-1", "First",
                   GURL(kShareUrl));
  store_->AddShare("share-2", "deletion-2", "conversation-share-2", "Second",
                   GURL(kShareUrl));

  store_->RemoveShare("share-1");

  std::vector<mojom::ConversationSharePtr> shares = GetShares();
  ASSERT_EQ(shares.size(), 1u);
  EXPECT_EQ(shares[0]->share_id, "share-2");
  EXPECT_FALSE(GetDeletionId("share-1").has_value());
}

TEST_F(ConversationShareStoreUnitTest, GetDeletionIdForUnknownShare) {
  EXPECT_FALSE(GetDeletionId("never-shared").has_value());
}

TEST_F(ConversationShareStoreUnitTest, PersistsAcrossStoreInstances) {
  store_->AddShare("share-1", "deletion-1", "conversation-share-1",
                   "My conversation", GURL(kShareUrl));

  CreateStore();

  std::vector<mojom::ConversationSharePtr> shares = GetShares();
  ASSERT_EQ(shares.size(), 1u);
  EXPECT_EQ(shares[0]->share_id, "share-1");
  EXPECT_EQ(shares[0]->conversation_uuid, "conversation-share-1");
  EXPECT_EQ(shares[0]->conversation_title, "My conversation");
  EXPECT_EQ(GetDeletionId("share-1"), "deletion-1");
  EXPECT_EQ(GetShareUrl("share-1"), GURL(kShareUrl));
}

TEST_F(ConversationShareStoreUnitTest, StoredRecordsAreNotReadableAsPlaintext) {
  store_->AddShare("share-1", "deletion-1", "conversation-share-1",
                   "My conversation", GURL(kShareUrl));

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
  store_->AddShare("share-1", "deletion-1", "conversation-share-1", "Old",
                   GURL(kShareUrl));
  task_environment_.FastForwardBy(base::Days(3));
  store_->AddShare("share-2", "deletion-2", "conversation-share-2", "New",
                   GURL(kShareUrl));
  const std::string stored_with_both =
      prefs_.GetString(prefs::kBraveAIChatConversationShares);

  // Destroy the store before advancing time: a live one purges on its own
  // timer, which would leave the load path with nothing to drop.
  store_.reset();
  task_environment_.FastForwardBy(
      base::Days(features::kAIChatConversationShareExpiryDays.Get()) -
      base::Days(2));
  CreateStore();

  // The server deletes shares after the expiry period, so the expired record is
  // dropped from disk on load, not merely filtered out of what is reported.
  EXPECT_NE(prefs_.GetString(prefs::kBraveAIChatConversationShares),
            stored_with_both);
  std::vector<mojom::ConversationSharePtr> shares = GetShares();
  ASSERT_EQ(shares.size(), 1u);
  EXPECT_EQ(shares[0]->share_id, "share-2");
}

TEST_F(ConversationShareStoreUnitTest, DropsExpiredRecordsWhileRunning) {
  store_->AddShare("share-1", "deletion-1", "conversation-share-1", "Old",
                   GURL(kShareUrl));

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

  store_->AddShare("share-1", "deletion-1", "conversation-share-1", "Shared",
                   GURL(kShareUrl));
  // While a share is live, a purge is queued for exactly when it expires.
  EXPECT_EQ(task_environment_.NextMainThreadPendingTaskDelay(),
            base::Days(features::kAIChatConversationShareExpiryDays.Get()));

  store_->RemoveShare("share-1");
  EXPECT_EQ(task_environment_.NextMainThreadPendingTaskDelay(),
            base::TimeDelta::Max());
}

TEST_F(ConversationShareStoreUnitTest, ExpiryPeriodIsConfigurable) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kAIChatConversationShare, {{"expiry_days", "30"}});

  store_->AddShare("share-1", "deletion-1", "conversation-share-1", "Old",
                   GURL(kShareUrl));

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

  // A value which can never be read back must not stop shares being recorded
  // from now on.
  store_->AddShare("share-1", "deletion-1", "conversation-share-1", "Shared",
                   GURL(kShareUrl));
  std::vector<mojom::ConversationSharePtr> shares = GetShares();
  ASSERT_EQ(shares.size(), 1u);
  EXPECT_EQ(shares[0]->share_id, "share-1");
}

TEST_F(ConversationShareStoreUnitTest, KeepsStoredDataWhenNoKeyIsAvailable) {
  FakeOSCryptAsync os_crypt;
  store_ = std::make_unique<ConversationShareStore>(&prefs_, &os_crypt);
  store_->AddShare("share-1", "deletion-1", "conversation-share-1", "Shared",
                   GURL(kShareUrl));
  const std::string stored =
      prefs_.GetString(prefs::kBraveAIChatConversationShares);
  ASSERT_FALSE(stored.empty());

  // A key can be unavailable for a session, e.g. a locked keyring, which says
  // nothing about whether what is stored is still good.
  os_crypt.encryptor().set_decryption_available_for_testing(false);
  store_ = std::make_unique<ConversationShareStore>(&prefs_, &os_crypt);

  EXPECT_TRUE(GetShares().empty());
  EXPECT_EQ(prefs_.GetString(prefs::kBraveAIChatConversationShares), stored);

  // Nor may a new share write over records which can't be read right now.
  store_->AddShare("share-2", "deletion-2", "conversation-share-2", "Shared",
                   GURL(kShareUrl));
  EXPECT_EQ(prefs_.GetString(prefs::kBraveAIChatConversationShares), stored);

  // They are readable again once the key is back.
  os_crypt.encryptor().set_decryption_available_for_testing(std::nullopt);
  store_ = std::make_unique<ConversationShareStore>(&prefs_, &os_crypt);
  std::vector<mojom::ConversationSharePtr> shares = GetShares();
  ASSERT_EQ(shares.size(), 1u);
  EXPECT_EQ(shares[0]->share_id, "share-1");
}

}  // namespace ai_chat
