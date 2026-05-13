/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_manage_profile_handler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted_memory.h"
#include "base/test/test_future.h"
#include "brave/browser/ui/webui/settings/brave_manage_profile.mojom.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_utils.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"

namespace {

// Test observer that captures `OnCustomAvatarChanged` events from the
// handler so tests can assert on the pushed state.
class TestSettingsUI
    : public brave_manage_profile::mojom::BraveManageProfileSettingsUI {
 public:
  TestSettingsUI() = default;
  ~TestSettingsUI() override = default;

  mojo::PendingRemote<brave_manage_profile::mojom::BraveManageProfileSettingsUI>
  BindAndGetRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  // Flushes the in-flight mojo messages so test assertions observe the
  // latest state pushed from the browser-side handler.
  void Flush() {
    if (receiver_.is_bound()) {
      receiver_.FlushForTesting();
    }
  }

  void OnCustomAvatarChanged(
      brave_manage_profile::mojom::CustomAvatarStatePtr state) override {
    last_state_ = std::move(state);
    ++change_count_;
  }

  const brave_manage_profile::mojom::CustomAvatarState* last_state() const {
    return last_state_.get();
  }

  int change_count() const { return change_count_; }

 private:
  mojo::Receiver<brave_manage_profile::mojom::BraveManageProfileSettingsUI>
      receiver_{this};
  brave_manage_profile::mojom::CustomAvatarStatePtr last_state_;
  int change_count_ = 0;
};

}  // namespace

class BraveManageProfileHandlerTest : public testing::Test {
 public:
  BraveManageProfileHandlerTest() = default;
  ~BraveManageProfileHandlerTest() override = default;

  void SetUp() override {
    profile_manager_ = std::make_unique<TestingProfileManager>(
        TestingBrowserProcess::GetGlobal());
    ASSERT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile("TestProfile");

    handler_ = std::make_unique<BraveManageProfileHandler>(profile_.get());
    handler_->BindUI(test_ui_.BindAndGetRemote());
  }

  void TearDown() override {
    handler_.reset();
    profile_ = nullptr;
    profile_manager_->DeleteTestingProfile("TestProfile");
    profile_manager_.reset();
  }

  Profile* profile() { return profile_; }

  ProfileAttributesEntry* entry() {
    return TestingBrowserProcess::GetGlobal()
        ->profile_manager()
        ->GetProfileAttributesStorage()
        .GetProfileAttributesWithPath(profile_->GetPath());
  }

  // Generates a valid PNG payload (base64-encoded) for a `side x side` test
  // image.
  std::string CreatePngBase64(int side) {
    gfx::Image image = gfx::test::CreateImage(side, side);
    scoped_refptr<base::RefCountedMemory> png_bytes = image.As1xPNGBytes();
    EXPECT_TRUE(png_bytes && png_bytes->size() > 0u);
    return base::Base64Encode(*png_bytes);
  }

  // Convenience wrappers that drive the public mojom interface and wait for
  // their reply via `base::test::TestFuture`.
  brave_manage_profile::mojom::CustomAvatarStatePtr GetCustomAvatar() {
    base::test::TestFuture<brave_manage_profile::mojom::CustomAvatarStatePtr>
        future;
    handler_->GetCustomAvatar(future.GetCallback());
    return future.Take();
  }

  struct SetResult {
    std::optional<brave_manage_profile::mojom::SetCustomAvatarError> error;
    brave_manage_profile::mojom::CustomAvatarStatePtr state;
  };

  SetResult SetCustomAvatar(const std::string& base64_payload) {
    base::test::TestFuture<
        std::optional<brave_manage_profile::mojom::SetCustomAvatarError>,
        brave_manage_profile::mojom::CustomAvatarStatePtr>
        future;
    handler_->SetCustomAvatar(base64_payload, future.GetCallback());
    auto [error, state] = future.Take();
    return {error, std::move(state)};
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
  raw_ptr<TestingProfile> profile_ = nullptr;
  TestSettingsUI test_ui_;
  std::unique_ptr<BraveManageProfileHandler> handler_;
};

// `GetCustomAvatar` returns an empty snapshot when no upload has happened.
TEST_F(BraveManageProfileHandlerTest, InitialStateHasNoCustomAvatar) {
  auto state = GetCustomAvatar();
  ASSERT_TRUE(state);
  EXPECT_FALSE(state->has_saved_avatar);
  EXPECT_FALSE(state->is_active);
  EXPECT_TRUE(state->data_url.empty());
}

// A valid PNG payload is decoded, persisted on the entry, and the response
// carries saved + active plus a data URL preview.
TEST_F(BraveManageProfileHandlerTest, SetWithValidPngResolvesAndStores) {
  auto result = SetCustomAvatar(CreatePngBase64(64));
  ASSERT_TRUE(result.state);
  EXPECT_FALSE(result.error.has_value());
  EXPECT_TRUE(result.state->has_saved_avatar);
  EXPECT_TRUE(result.state->is_active);
  EXPECT_TRUE(result.state->data_url.starts_with("data:image/png;base64,"));

  ASSERT_TRUE(entry());
  EXPECT_TRUE(entry()->HasBraveCustomAvatar());
  EXPECT_TRUE(entry()->IsUsingBraveCustomAvatar());
}

// Rejection cases for `SetCustomAvatar`: each payload must produce the
// matching error and leave the profile entry untouched.
struct RejectCase {
  // Human-readable test suffix; also the GTest case name printer.
  std::string name;
  // Payload passed verbatim to `SetCustomAvatar` (base64 already applied
  // where the failure mode is "decodes as base64 but isn't an image").
  std::string payload;
  brave_manage_profile::mojom::SetCustomAvatarError expected_error;
};

class BraveManageProfileHandlerRejectTest
    : public BraveManageProfileHandlerTest,
      public testing::WithParamInterface<RejectCase> {};

TEST_P(BraveManageProfileHandlerRejectTest, SetIsRejected) {
  const RejectCase& test_case = GetParam();
  auto result = SetCustomAvatar(test_case.payload);
  ASSERT_TRUE(result.error.has_value());
  EXPECT_EQ(test_case.expected_error, *result.error);
  ASSERT_TRUE(entry());
  EXPECT_FALSE(entry()->HasBraveCustomAvatar());
  EXPECT_FALSE(entry()->IsUsingBraveCustomAvatar());
}

INSTANTIATE_TEST_SUITE_P(
    All,
    BraveManageProfileHandlerRejectTest,
    testing::Values(
        // Empty payload is rejected without invoking the decoder.
        RejectCase{"EmptyPayload", std::string(),
                   brave_manage_profile::mojom::SetCustomAvatarError::kEmpty},
        // Non-base64 garbage is rejected before the decoder is reached.
        RejectCase{
            "InvalidBase64", "!!! not base64 !!!",
            brave_manage_profile::mojom::SetCustomAvatarError::kInvalidBase64},
        // Base64-encoded random bytes decode fine as base64 but fail the
        // image decode step.
        RejectCase{
            "NonImageBytes",
            base::Base64Encode(
                std::string("\x01\x02\x03\x04not a PNG\x05\x06\x07", 18)),
            brave_manage_profile::mojom::SetCustomAvatarError::kDecodeFailed}),
    [](const testing::TestParamInfo<RejectCase>& info) {
      return info.param.name;
    });

// `RemoveCustomAvatar` clears the stored avatar and pushes the change to the
// bound `BraveManageProfileSettingsUI`.
TEST_F(BraveManageProfileHandlerTest, RemoveClearsAvatarAndNotifiesUi) {
  ASSERT_FALSE(SetCustomAvatar(CreatePngBase64(48)).error.has_value());
  ASSERT_TRUE(entry());
  ASSERT_TRUE(entry()->HasBraveCustomAvatar());
  ASSERT_TRUE(entry()->IsUsingBraveCustomAvatar());

  handler_->RemoveCustomAvatar();
  content::RunAllTasksUntilIdle();
  test_ui_.Flush();

  EXPECT_FALSE(entry()->HasBraveCustomAvatar());
  EXPECT_FALSE(entry()->IsUsingBraveCustomAvatar());
  ASSERT_TRUE(test_ui_.last_state());
  EXPECT_FALSE(test_ui_.last_state()->has_saved_avatar);
  EXPECT_FALSE(test_ui_.last_state()->is_active);
}

// `ActivateCustomAvatar` re-selects a saved but inactive custom avatar on
// the profile entry.
TEST_F(BraveManageProfileHandlerTest, ActivateRestoresCustomAvatar) {
  ASSERT_FALSE(SetCustomAvatar(CreatePngBase64(32)).error.has_value());
  ASSERT_TRUE(entry());
  ASSERT_TRUE(entry()->IsUsingBraveCustomAvatar());

  entry()->DeactivateBraveCustomAvatar();
  ASSERT_TRUE(entry()->HasBraveCustomAvatar());
  ASSERT_FALSE(entry()->IsUsingBraveCustomAvatar());

  handler_->ActivateCustomAvatar();
  content::RunAllTasksUntilIdle();

  EXPECT_TRUE(entry()->HasBraveCustomAvatar());
  EXPECT_TRUE(entry()->IsUsingBraveCustomAvatar());
}
