// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// AIChatService conversation sharing tests. These live in their own file (built
// only when use_blink is true) because verifying the clipboard copy requires
// ui::TestClipboard, which the clipboard test support target only builds when
// use_blink is true (so it is unavailable on non-blink builds such as iOS). The
// URL-building logic under test is platform-agnostic, so exercising it on the
// blink-based platforms is sufficient.

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/conversation_share_manager.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/network_context_getter.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/clipboard_buffer.h"
#include "ui/base/clipboard/test/test_clipboard.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

class MockAIChatCredentialManager : public AIChatCredentialManager {
 public:
  using AIChatCredentialManager::AIChatCredentialManager;
  MOCK_METHOD(void,
              GetPremiumStatus,
              (mojom::Service::GetPremiumStatusCallback callback),
              (override));
};

// Returns a fixed share result without any network access, so tests can
// exercise AIChatService's post-upload handling (appending the key fragment and
// copying to the clipboard) without hitting the sharing server.
class FakeConversationShareManager : public ConversationShareManager {
 public:
  FakeConversationShareManager() : ConversationShareManager(nullptr) {}
  ~FakeConversationShareManager() override = default;

  void ShareConversation(const std::string& encrypted_contents,
                         ShareConversationCallback callback) override {
    last_encrypted_contents = encrypted_contents;
    std::move(callback).Run(share_result);
  }

  void DeleteShare(const std::string& deletion_id,
                   DeleteShareCallback callback) override {
    last_deletion_id = deletion_id;
    std::move(callback).Run(delete_succeeds);
  }

  // What the fake server "returns" (the viewer URL has no decryption key
  // fragment).
  std::optional<ConversationShareResult> share_result;
  bool delete_succeeds = true;
  // Captures what was uploaded, to assert the key fragment never reaches here.
  std::string last_encrypted_contents;
  std::string last_deletion_id;
};

ConversationShareResult MakeShareResult() {
  return ConversationShareResult{
      .viewer_url = GURL("https://leo-ai.brave.app/sharing/test-share-id"),
      .share_id = "test-share-id",
      .deletion_id = "test-deletion-id"};
}

}  // namespace

class AIChatServiceShareConversationTest : public testing::Test {
 public:
  AIChatServiceShareConversationTest() = default;

  void SetUp() override {
    CHECK(temp_directory_.CreateUniqueTempDir());
    prefs::RegisterProfilePrefs(prefs_.registry());
    prefs::RegisterLocalStatePrefs(local_state_.registry());
    ModelService::RegisterProfilePrefs(prefs_.registry());

    // These tests exercise only ShareConversation, which is independent of
    // conversation storage. Keep storage disabled so the service starts no
    // async database work that would otherwise need draining at teardown.
    prefs_.SetBoolean(prefs::kBraveChatStorageEnabled, false);

    os_crypt_ = os_crypt_async::GetTestOSCryptAsyncForTesting(
        /*is_sync_for_unittests=*/true);
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    model_service_ = std::make_unique<ModelService>(
        &prefs_, os_crypt_.get(), network::NetworkContextGetter());
    tab_tracker_service_ = std::make_unique<TabTrackerService>();

    auto credential_manager =
        std::make_unique<testing::NiceMock<MockAIChatCredentialManager>>(
            base::NullCallback(), &local_state_);
    ON_CALL(*credential_manager, GetPremiumStatus(testing::_))
        .WillByDefault([](mojom::Service::GetPremiumStatusCallback callback) {
          std::move(callback).Run(mojom::PremiumStatus::Active,
                                  mojom::PremiumInfo::New());
        });

    ai_chat_service_ = std::make_unique<AIChatService>(
        model_service_.get(), tab_tracker_service_.get(),
        std::move(credential_manager), &prefs_, /*ai_chat_metrics=*/nullptr,
        os_crypt_.get(), shared_url_loader_factory_, /*channel_string=*/"",
        temp_directory_.GetPath());
  }

  void TearDown() override { ai_chat_service_.reset(); }

  // The share store replies on this sequence in the order it was asked, so a
  // completed listing means anything requested of it earlier has finished.
  void WaitForShareStore() {
    base::test::TestFuture<std::vector<mojom::ConversationSharePtr>> future;
    ai_chat_service_->GetConversationShares(future.GetCallback());
    ASSERT_TRUE(future.Wait());
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_directory_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<ModelService> model_service_;
  std::unique_ptr<TabTrackerService> tab_tracker_service_;
  std::unique_ptr<AIChatService> ai_chat_service_;
};

TEST_F(AIChatServiceShareConversationTest, ReturnsFullUrlWithKeyFragment) {
  auto fake_share_manager = std::make_unique<FakeConversationShareManager>();
  fake_share_manager->share_result = MakeShareResult();
  auto* fake_share_manager_ptr = fake_share_manager.get();
  ai_chat_service_->SetConversationShareManagerForTesting(
      std::move(fake_share_manager));

  base::test::TestFuture<const std::optional<GURL>&> future;
  ai_chat_service_->ShareConversation(
      "ciphertext-blob", "url-safe-key-fragment", "conversation-uuid",
      "Conversation title",
      /*copy_to_clipboard=*/false, future.GetCallback());

  const std::optional<GURL>& result = future.Get();
  ASSERT_TRUE(result.has_value());
  // The full shareable link is the server's viewer URL with the decryption key
  // appended as a fragment.
  EXPECT_EQ(
      result->spec(),
      "https://leo-ai.brave.app/sharing/test-share-id#url-safe-key-fragment");
  // Only the ciphertext reaches the network-facing share manager; the key
  // fragment stays in the browser process and is never uploaded.
  EXPECT_EQ(fake_share_manager_ptr->last_encrypted_contents, "ciphertext-blob");
}

TEST_F(AIChatServiceShareConversationTest, ReturnsNulloptWhenSharingFails) {
  auto fake_share_manager = std::make_unique<FakeConversationShareManager>();
  // A null result simulates a failed upload (network error, bad response).
  fake_share_manager->share_result = std::nullopt;
  ai_chat_service_->SetConversationShareManagerForTesting(
      std::move(fake_share_manager));

  base::test::TestFuture<const std::optional<GURL>&> future;
  ai_chat_service_->ShareConversation(
      "ciphertext-blob", "url-safe-key-fragment", "conversation-uuid",
      "Conversation title",
      /*copy_to_clipboard=*/true, future.GetCallback());

  EXPECT_FALSE(future.Get().has_value());
}

TEST_F(AIChatServiceShareConversationTest,
       CopiesFullLinkToClipboardWhenRequested) {
  auto fake_share_manager = std::make_unique<FakeConversationShareManager>();
  fake_share_manager->share_result = MakeShareResult();
  ai_chat_service_->SetConversationShareManagerForTesting(
      std::move(fake_share_manager));

  ui::TestClipboard* clipboard = ui::TestClipboard::CreateForCurrentThread();

  base::test::TestFuture<const std::optional<GURL>&> future;
  ai_chat_service_->ShareConversation(
      "ciphertext-blob", "url-safe-key-fragment", "conversation-uuid",
      "Conversation title",
      /*copy_to_clipboard=*/true, future.GetCallback());
  std::optional<GURL> result = future.Get();

  base::test::TestFuture<std::u16string> clipboard_future;
  clipboard->ReadText(ui::ClipboardBuffer::kCopyPaste,
                      /*data_dst=*/std::nullopt,
                      clipboard_future.GetCallback());
  std::u16string clipboard_text = clipboard_future.Get();

  ui::Clipboard::DestroyClipboardForCurrentThread();

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(
      result->spec(),
      "https://leo-ai.brave.app/sharing/test-share-id#url-safe-key-fragment");
  // The browser process copied the full shareable link, not the bare viewer
  // URL, so the recipient can decrypt the conversation.
  EXPECT_EQ(
      base::UTF16ToUTF8(clipboard_text),
      "https://leo-ai.brave.app/sharing/test-share-id#url-safe-key-fragment");
}

TEST_F(AIChatServiceShareConversationTest,
       DoesNotCopyToClipboardWhenNotRequested) {
  auto fake_share_manager = std::make_unique<FakeConversationShareManager>();
  fake_share_manager->share_result = MakeShareResult();
  ai_chat_service_->SetConversationShareManagerForTesting(
      std::move(fake_share_manager));

  ui::TestClipboard* clipboard = ui::TestClipboard::CreateForCurrentThread();

  base::test::TestFuture<const std::optional<GURL>&> future;
  ai_chat_service_->ShareConversation(
      "ciphertext-blob", "url-safe-key-fragment", "conversation-uuid",
      "Conversation title",
      /*copy_to_clipboard=*/false, future.GetCallback());
  std::optional<GURL> result = future.Get();

  base::test::TestFuture<std::u16string> clipboard_future;
  clipboard->ReadText(ui::ClipboardBuffer::kCopyPaste,
                      /*data_dst=*/std::nullopt,
                      clipboard_future.GetCallback());
  std::u16string clipboard_text = clipboard_future.Get();

  ui::Clipboard::DestroyClipboardForCurrentThread();

  // The URL is still returned, but nothing is written to the clipboard when the
  // caller does not request it.
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(clipboard_text.empty());
}

TEST_F(AIChatServiceShareConversationTest, RecordsShareSoItCanBeManaged) {
  auto fake_share_manager = std::make_unique<FakeConversationShareManager>();
  fake_share_manager->share_result = MakeShareResult();
  ai_chat_service_->SetConversationShareManagerForTesting(
      std::move(fake_share_manager));

  base::test::TestFuture<const std::optional<GURL>&> share_future;
  ai_chat_service_->ShareConversation(
      "ciphertext-blob", "url-safe-key-fragment", "conversation-uuid",
      "Conversation title",
      /*copy_to_clipboard=*/false, share_future.GetCallback());
  ASSERT_TRUE(share_future.Get().has_value());

  base::test::TestFuture<std::vector<mojom::ConversationSharePtr>>
      shares_future;
  ai_chat_service_->GetConversationShares(shares_future.GetCallback());
  const std::vector<mojom::ConversationSharePtr>& shares = shares_future.Get();

  ASSERT_EQ(shares.size(), 1u);
  EXPECT_EQ(shares[0]->share_id, "test-share-id");
  // The conversation the share was made from is recorded too, so the share can
  // be related back to it.
  EXPECT_EQ(shares[0]->conversation_uuid, "conversation-uuid");
  EXPECT_EQ(shares[0]->conversation_title, "Conversation title");
}

TEST_F(AIChatServiceShareConversationTest, CopiesRecordedShareLinkAgain) {
  auto fake_share_manager = std::make_unique<FakeConversationShareManager>();
  fake_share_manager->share_result = MakeShareResult();
  ai_chat_service_->SetConversationShareManagerForTesting(
      std::move(fake_share_manager));

  base::test::TestFuture<const std::optional<GURL>&> share_future;
  ai_chat_service_->ShareConversation(
      "ciphertext-blob", "url-safe-key-fragment", "conversation-uuid",
      "Conversation title",
      /*copy_to_clipboard=*/false, share_future.GetCallback());
  ASSERT_TRUE(share_future.Get().has_value());

  ui::TestClipboard* clipboard = ui::TestClipboard::CreateForCurrentThread();
  ai_chat_service_->CopyConversationShareLink("test-share-id");
  WaitForShareStore();

  base::test::TestFuture<std::u16string> clipboard_future;
  clipboard->ReadText(ui::ClipboardBuffer::kCopyPaste,
                      /*data_dst=*/std::nullopt,
                      clipboard_future.GetCallback());
  std::u16string clipboard_text = clipboard_future.Get();
  ui::Clipboard::DestroyClipboardForCurrentThread();

  // The recorded link includes the key fragment, so the user can share it with
  // someone else without re-uploading the conversation.
  EXPECT_EQ(
      base::UTF16ToUTF8(clipboard_text),
      "https://leo-ai.brave.app/sharing/test-share-id#url-safe-key-fragment");
}

TEST_F(AIChatServiceShareConversationTest, DoesNotCopyLinkForUnknownShare) {
  ui::TestClipboard* clipboard = ui::TestClipboard::CreateForCurrentThread();
  ai_chat_service_->CopyConversationShareLink("never-shared");
  WaitForShareStore();

  base::test::TestFuture<std::u16string> clipboard_future;
  clipboard->ReadText(ui::ClipboardBuffer::kCopyPaste,
                      /*data_dst=*/std::nullopt,
                      clipboard_future.GetCallback());
  std::u16string clipboard_text = clipboard_future.Get();
  ui::Clipboard::DestroyClipboardForCurrentThread();

  // Nothing is recorded for the share, so there is no link to put anywhere.
  EXPECT_TRUE(clipboard_text.empty());
}

TEST_F(AIChatServiceShareConversationTest, DeleteConversationShareForgetsIt) {
  auto fake_share_manager = std::make_unique<FakeConversationShareManager>();
  fake_share_manager->share_result = MakeShareResult();
  auto* fake_share_manager_ptr = fake_share_manager.get();
  ai_chat_service_->SetConversationShareManagerForTesting(
      std::move(fake_share_manager));

  base::test::TestFuture<const std::optional<GURL>&> share_future;
  ai_chat_service_->ShareConversation(
      "ciphertext-blob", "url-safe-key-fragment", "conversation-uuid",
      "Conversation title",
      /*copy_to_clipboard=*/false, share_future.GetCallback());
  ASSERT_TRUE(share_future.Get().has_value());

  base::test::TestFuture<bool> delete_future;
  ai_chat_service_->DeleteConversationShare("test-share-id",
                                            delete_future.GetCallback());
  EXPECT_TRUE(delete_future.Get());
  // The server is authorized with the deletion id it handed out, not the share
  // id from the link.
  EXPECT_EQ(fake_share_manager_ptr->last_deletion_id, "test-deletion-id");

  base::test::TestFuture<std::vector<mojom::ConversationSharePtr>>
      shares_future;
  ai_chat_service_->GetConversationShares(shares_future.GetCallback());
  EXPECT_TRUE(shares_future.Get().empty());
}

TEST_F(AIChatServiceShareConversationTest, KeepsRecordWhenServerDeleteFails) {
  auto fake_share_manager = std::make_unique<FakeConversationShareManager>();
  fake_share_manager->share_result = MakeShareResult();
  fake_share_manager->delete_succeeds = false;
  ai_chat_service_->SetConversationShareManagerForTesting(
      std::move(fake_share_manager));

  base::test::TestFuture<const std::optional<GURL>&> share_future;
  ai_chat_service_->ShareConversation(
      "ciphertext-blob", "url-safe-key-fragment", "conversation-uuid",
      "Conversation title",
      /*copy_to_clipboard=*/false, share_future.GetCallback());
  ASSERT_TRUE(share_future.Get().has_value());

  base::test::TestFuture<bool> delete_future;
  ai_chat_service_->DeleteConversationShare("test-share-id",
                                            delete_future.GetCallback());
  EXPECT_FALSE(delete_future.Get());

  // The share still exists on the server, so the user keeps a way to retry.
  base::test::TestFuture<std::vector<mojom::ConversationSharePtr>>
      shares_future;
  ai_chat_service_->GetConversationShares(shares_future.GetCallback());
  EXPECT_EQ(shares_future.Get().size(), 1u);
}

TEST_F(AIChatServiceShareConversationTest, DeleteUnknownShareSucceeds) {
  auto fake_share_manager = std::make_unique<FakeConversationShareManager>();
  auto* fake_share_manager_ptr = fake_share_manager.get();
  ai_chat_service_->SetConversationShareManagerForTesting(
      std::move(fake_share_manager));

  base::test::TestFuture<bool> delete_future;
  ai_chat_service_->DeleteConversationShare("never-shared",
                                            delete_future.GetCallback());

  // Nothing is known about the share, so there is nothing to ask the server to
  // delete and nothing for the user to retry.
  EXPECT_TRUE(delete_future.Get());
  EXPECT_TRUE(fake_share_manager_ptr->last_deletion_id.empty());
}

}  // namespace ai_chat
