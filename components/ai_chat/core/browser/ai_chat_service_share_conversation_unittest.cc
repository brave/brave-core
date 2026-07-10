// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// AIChatService::ShareConversation tests. These live in their own file (built
// only when use_blink is true) because verifying the clipboard copy requires
// ui::TestClipboard, which the clipboard test support target only builds when
// use_blink is true (so it is unavailable on non-blink builds such as iOS). The
// URL-building logic under test is platform-agnostic, so exercising it on the
// blink-based platforms is sufficient.

#include <memory>
#include <optional>
#include <string>

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

// Returns a fixed viewer URL without any network access, so tests can exercise
// AIChatService's post-upload handling (appending the key fragment and copying
// to the clipboard) without hitting the sharing server.
class FakeConversationShareManager : public ConversationShareManager {
 public:
  FakeConversationShareManager() : ConversationShareManager(nullptr) {}
  ~FakeConversationShareManager() override = default;

  void ShareConversation(const std::string& encrypted_contents,
                         ShareConversationCallback callback) override {
    last_encrypted_contents = encrypted_contents;
    std::move(callback).Run(viewer_url);
  }

  // The URL the fake server "returns" (without the decryption key fragment).
  std::optional<GURL> viewer_url;
  // Captures what was uploaded, to assert the key fragment never reaches here.
  std::string last_encrypted_contents;
};

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
  fake_share_manager->viewer_url =
      GURL("https://leo-ai.brave.app/sharing/test-share-id");
  auto* fake_share_manager_ptr = fake_share_manager.get();
  ai_chat_service_->SetConversationShareManagerForTesting(
      std::move(fake_share_manager));

  base::test::TestFuture<const std::optional<GURL>&> future;
  ai_chat_service_->ShareConversation(
      "ciphertext-blob", "url-safe-key-fragment",
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
  // A null viewer URL simulates a failed upload (network error, bad response).
  fake_share_manager->viewer_url = std::nullopt;
  ai_chat_service_->SetConversationShareManagerForTesting(
      std::move(fake_share_manager));

  base::test::TestFuture<const std::optional<GURL>&> future;
  ai_chat_service_->ShareConversation(
      "ciphertext-blob", "url-safe-key-fragment",
      /*copy_to_clipboard=*/true, future.GetCallback());

  EXPECT_FALSE(future.Get().has_value());
}

TEST_F(AIChatServiceShareConversationTest,
       CopiesFullLinkToClipboardWhenRequested) {
  auto fake_share_manager = std::make_unique<FakeConversationShareManager>();
  fake_share_manager->viewer_url =
      GURL("https://leo-ai.brave.app/sharing/test-share-id");
  ai_chat_service_->SetConversationShareManagerForTesting(
      std::move(fake_share_manager));

  ui::TestClipboard* clipboard = ui::TestClipboard::CreateForCurrentThread();

  base::test::TestFuture<const std::optional<GURL>&> future;
  ai_chat_service_->ShareConversation(
      "ciphertext-blob", "url-safe-key-fragment",
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

}  // namespace ai_chat
