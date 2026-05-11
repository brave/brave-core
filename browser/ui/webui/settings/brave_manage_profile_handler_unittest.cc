/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_manage_profile_handler.h"

#include <memory>
#include <string>

#include "base/base64.h"
#include "base/containers/adapters.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted_memory.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_utils.h"
#include "content/public/test/test_web_ui.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"

namespace {

// Helper that exposes the protected `set_web_ui` setter so tests can wire up
// a `TestWebUI` to the handler.
class TestableBraveManageProfileHandler : public BraveManageProfileHandler {
 public:
  using BraveManageProfileHandler::BraveManageProfileHandler;
  using BraveManageProfileHandler::set_web_ui;
};

// Returns the most recent payload published to `listener_name` by the
// handler via `FireWebUIListener`, or `nullptr` if none was sent.
const base::DictValue* LastWebUIListenerPayload(
    const content::TestWebUI& web_ui,
    const std::string& listener_name) {
  for (const std::unique_ptr<content::TestWebUI::CallData>& data :
       base::Reversed(web_ui.call_data())) {
    if (data->function_name() != "cr.webUIListenerCallback" || !data->arg1() ||
        !data->arg1()->is_string() ||
        data->arg1()->GetString() != listener_name) {
      continue;
    }
    return data->arg2() ? data->arg2()->GetIfDict() : nullptr;
  }
  return nullptr;
}

// Returns the resolved/rejected payload for the most recent
// `cr.webUIResponse` call carrying `callback_id`, or nullptr.
struct WebUIResponse {
  bool success = false;
  const base::Value* payload = nullptr;
};
std::optional<WebUIResponse> FindWebUIResponse(const content::TestWebUI& web_ui,
                                               const std::string& callback_id) {
  for (const std::unique_ptr<content::TestWebUI::CallData>& data :
       base::Reversed(web_ui.call_data())) {
    if (data->function_name() != "cr.webUIResponse" || !data->arg1() ||
        !data->arg1()->is_string() ||
        data->arg1()->GetString() != callback_id) {
      continue;
    }
    WebUIResponse response;
    response.success =
        data->arg2() && data->arg2()->is_bool() && data->arg2()->GetBool();
    response.payload = data->arg3();
    return response;
  }
  return std::nullopt;
}

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

    web_contents_ = content::WebContents::Create(
        content::WebContents::CreateParams(profile_));
    web_ui_.set_web_contents(web_contents_.get());

    handler_ =
        std::make_unique<TestableBraveManageProfileHandler>(profile_.get());
    handler_->set_web_ui(&web_ui_);
    handler_->RegisterMessages();
  }

  void TearDown() override {
    handler_->set_web_ui(nullptr);
    handler_.reset();
    web_contents_.reset();
    profile_ = nullptr;
    profile_manager_->DeleteTestingProfile("TestProfile");
    profile_manager_.reset();
  }

  content::TestWebUI* web_ui() { return &web_ui_; }
  Profile* profile() { return profile_; }

  ProfileAttributesEntry* entry() {
    return TestingBrowserProcess::GetGlobal()
        ->profile_manager()
        ->GetProfileAttributesStorage()
        .GetProfileAttributesWithPath(profile_->GetPath());
  }

  void SendMessage(const std::string& message, base::ListValue args) {
    web_ui_.HandleReceivedMessage(message, args);
  }

  void GetCustomAvatar(const std::string& callback_id) {
    base::ListValue args;
    args.Append(callback_id);
    SendMessage("getProfileCustomAvatar", std::move(args));
  }

  void SetCustomAvatar(const std::string& callback_id,
                       const std::string& base64_payload) {
    base::ListValue args;
    args.Append(callback_id);
    args.Append(base64_payload);
    SendMessage("setProfileCustomAvatar", std::move(args));
  }

  void RemoveCustomAvatar() {
    SendMessage("removeProfileCustomAvatar", base::ListValue());
  }

  void ActivateCustomAvatar() {
    SendMessage("activateProfileCustomAvatar", base::ListValue());
  }

  // Generates a valid PNG payload (base64-encoded) for a `side x side` test
  // image.
  std::string CreatePngBase64(int side) {
    gfx::Image image = gfx::test::CreateImage(side, side);
    scoped_refptr<base::RefCountedMemory> png_bytes = image.As1xPNGBytes();
    EXPECT_TRUE(png_bytes && png_bytes->size() > 0u);
    return base::Base64Encode(*png_bytes);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
  raw_ptr<TestingProfile> profile_ = nullptr;
  std::unique_ptr<content::WebContents> web_contents_;
  content::TestWebUI web_ui_;
  std::unique_ptr<TestableBraveManageProfileHandler> handler_;
};

// `getProfileCustomAvatar` resolves with no saved custom avatar when none
// has been uploaded yet.
TEST_F(BraveManageProfileHandlerTest, InitialStateHasNoCustomAvatar) {
  GetCustomAvatar("cb-initial");

  auto response = FindWebUIResponse(*web_ui(), "cb-initial");
  ASSERT_TRUE(response.has_value());
  EXPECT_TRUE(response->success);
  ASSERT_TRUE(response->payload && response->payload->is_dict());
  const base::DictValue* dict = response->payload->GetIfDict();
  ASSERT_TRUE(dict->FindBool("hasSavedAvatar").has_value());
  EXPECT_FALSE(dict->FindBool("hasSavedAvatar").value());
  ASSERT_TRUE(dict->FindBool("isActive").has_value());
  EXPECT_FALSE(dict->FindBool("isActive").value());
}

// A valid PNG payload is decoded, persisted on the entry, and the
// resolved response carries saved + active plus a data URL preview.
TEST_F(BraveManageProfileHandlerTest, SetWithValidPngResolvesAndStores) {
  SetCustomAvatar("cb-set", CreatePngBase64(64));

  // Decoding is async (through the data_decoder service); pump tasks until
  // the response makes it back to the WebUI.
  content::RunAllTasksUntilIdle();

  auto response = FindWebUIResponse(*web_ui(), "cb-set");
  ASSERT_TRUE(response.has_value());
  EXPECT_TRUE(response->success);
  ASSERT_TRUE(response->payload && response->payload->is_dict());
  const base::DictValue* dict = response->payload->GetIfDict();
  EXPECT_TRUE(dict->FindBool("hasSavedAvatar").value_or(false));
  EXPECT_TRUE(dict->FindBool("isActive").value_or(false));
  const std::string* data_url = dict->FindString("dataUrl");
  ASSERT_TRUE(data_url);
  EXPECT_TRUE(data_url->starts_with("data:image/png;base64,"));

  ASSERT_TRUE(entry());
  EXPECT_TRUE(entry()->HasBraveCustomAvatar());
  EXPECT_TRUE(entry()->IsUsingBraveCustomAvatar());
}

// An empty payload is rejected without invoking the decoder.
TEST_F(BraveManageProfileHandlerTest, SetWithEmptyPayloadIsRejected) {
  SetCustomAvatar("cb-empty", std::string());

  auto response = FindWebUIResponse(*web_ui(), "cb-empty");
  ASSERT_TRUE(response.has_value());
  EXPECT_FALSE(response->success);
  ASSERT_TRUE(response->payload && response->payload->is_string());
  EXPECT_EQ("empty", response->payload->GetString());
  ASSERT_TRUE(entry());
  EXPECT_FALSE(entry()->HasBraveCustomAvatar());
  EXPECT_FALSE(entry()->IsUsingBraveCustomAvatar());
}

// Non-base64 garbage is rejected before the decoder is reached.
TEST_F(BraveManageProfileHandlerTest, SetWithInvalidBase64IsRejected) {
  SetCustomAvatar("cb-garbage", "!!! not base64 !!!");

  auto response = FindWebUIResponse(*web_ui(), "cb-garbage");
  ASSERT_TRUE(response.has_value());
  EXPECT_FALSE(response->success);
  ASSERT_TRUE(response->payload && response->payload->is_string());
  EXPECT_EQ("invalid-base64", response->payload->GetString());
  ASSERT_TRUE(entry());
  EXPECT_FALSE(entry()->HasBraveCustomAvatar());
  EXPECT_FALSE(entry()->IsUsingBraveCustomAvatar());
}

// Base64-encoded random bytes decode fine as base64 but fail the image
// decode step.
TEST_F(BraveManageProfileHandlerTest, SetWithNonImageBytesIsRejected) {
  const std::string garbage_bytes("\x01\x02\x03\x04not a PNG\x05\x06\x07", 18);
  SetCustomAvatar("cb-not-image", base::Base64Encode(garbage_bytes));

  content::RunAllTasksUntilIdle();

  auto response = FindWebUIResponse(*web_ui(), "cb-not-image");
  ASSERT_TRUE(response.has_value());
  EXPECT_FALSE(response->success);
  ASSERT_TRUE(response->payload && response->payload->is_string());
  EXPECT_EQ("decode-failed", response->payload->GetString());
  ASSERT_TRUE(entry());
  EXPECT_FALSE(entry()->HasBraveCustomAvatar());
  EXPECT_FALSE(entry()->IsUsingBraveCustomAvatar());
}

// `removeProfileCustomAvatar` clears the stored avatar and notifies the
// front-end via the `brave-custom-avatar-changed` web UI listener.
TEST_F(BraveManageProfileHandlerTest, RemoveClearsAvatarAndFiresListener) {
  SetCustomAvatar("cb-prepare", CreatePngBase64(48));
  content::RunAllTasksUntilIdle();
  ASSERT_TRUE(entry());
  ASSERT_TRUE(entry()->HasBraveCustomAvatar());
  ASSERT_TRUE(entry()->IsUsingBraveCustomAvatar());

  web_ui()->ClearTrackedCalls();
  RemoveCustomAvatar();
  content::RunAllTasksUntilIdle();

  EXPECT_FALSE(entry()->HasBraveCustomAvatar());
  EXPECT_FALSE(entry()->IsUsingBraveCustomAvatar());
  const base::DictValue* listener_payload =
      LastWebUIListenerPayload(*web_ui(), "brave-custom-avatar-changed");
  ASSERT_TRUE(listener_payload);
  EXPECT_FALSE(listener_payload->FindBool("hasSavedAvatar").value_or(true));
  EXPECT_FALSE(listener_payload->FindBool("isActive").value_or(true));
}

// `activateProfileCustomAvatar` re-selects a saved but inactive custom
// avatar on the profile entry.
TEST_F(BraveManageProfileHandlerTest, ActivateRestoresCustomAvatar) {
  SetCustomAvatar("cb-act", CreatePngBase64(32));
  content::RunAllTasksUntilIdle();
  ASSERT_TRUE(entry());
  ASSERT_TRUE(entry()->IsUsingBraveCustomAvatar());

  entry()->DeactivateBraveCustomAvatar();
  ASSERT_TRUE(entry()->HasBraveCustomAvatar());
  ASSERT_FALSE(entry()->IsUsingBraveCustomAvatar());

  ActivateCustomAvatar();
  content::RunAllTasksUntilIdle();

  EXPECT_TRUE(entry()->HasBraveCustomAvatar());
  EXPECT_TRUE(entry()->IsUsingBraveCustomAvatar());
}
