/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_sync_handler.h"

#include <memory>
#include <string>

#include "base/values.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_ui.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/test/clipboard_test_util.h"
#include "ui/base/clipboard/test/test_clipboard.h"

using ui::Clipboard;

namespace {

// TestClipboard's IsMarkedByOriginatorAsConfidential() always returns false,
// so this test needs to track the confidential privacy bit itself instead of
// relying on the real platform clipboard (mac only). Using the real
// NSPasteboard here made this test flaky in CI, since it's a resource shared
// across every process on the machine.
class ConfidentialTrackingTestClipboard : public ui::TestClipboard {
 public:
  void WritePortableAndPlatformRepresentations(
      ui::ClipboardBuffer buffer,
      const ObjectMap& objects,
      const std::vector<RawData>& raw_objects,
      std::vector<PlatformRepresentation> platform_representations,
      std::unique_ptr<ui::DataTransferEndpoint> data_src,
      uint32_t privacy_types) override {
    is_confidential_ = (privacy_types & Clipboard::kNoDisplay) != 0;
    ui::TestClipboard::WritePortableAndPlatformRepresentations(
        buffer, objects, raw_objects, std::move(platform_representations),
        std::move(data_src), privacy_types);
  }

  bool IsMarkedByOriginatorAsConfidential() const override {
    return is_confidential_;
  }

 private:
  bool is_confidential_ = false;
};

}  // namespace

class BraveSyncHandlerUnittest : public testing::Test {
 public:
  BraveSyncHandlerUnittest() {
    TestingProfile::Builder builder;

    profile_ = builder.Build();
    web_contents_ = content::WebContents::Create(
        content::WebContents::CreateParams(profile_.get()));

    test_web_ui_.set_web_contents(web_contents_.get());

    handler_ = std::make_unique<BraveSyncHandler>();
    handler_->set_web_ui(&test_web_ui_);
    handler_->RegisterMessages();
  }
  ~BraveSyncHandlerUnittest() override {
    // The test handler unusually owns its own TestWebUI, so we make sure to
    // unbind it from the base class before the derived class is destroyed.
    handler_->set_web_ui(nullptr);
    handler_.reset();
  }

  void SetUp() override {
    Clipboard::SetClipboardForCurrentThread(
        std::make_unique<ConfidentialTrackingTestClipboard>());
    clipboard_ = Clipboard::GetForCurrentThread();
  }

  void TearDown() override {
    ASSERT_EQ(Clipboard::GetForCurrentThread(), clipboard_.ExtractAsDangling());
    Clipboard::DestroyClipboardForCurrentThread();
  }

 protected:
  void CallHandleCopySyncCodeToClipboard(const base::ListValue& args);
  Clipboard& clipboard() { return *clipboard_; }
  content::TestWebUI* web_ui() { return &test_web_ui_; }

 private:
  std::unique_ptr<BraveSyncHandler> handler_;
  content::BrowserTaskEnvironment browser_task_environment;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<content::WebContents> web_contents_;
  content::TestWebUI test_web_ui_;
  // Clipboard has a protected destructor, so scoped_ptr doesn't work here.
  raw_ptr<Clipboard> clipboard_ = nullptr;
};

void BraveSyncHandlerUnittest::CallHandleCopySyncCodeToClipboard(
    const base::ListValue& args) {
  handler_->HandleCopySyncCodeToClipboard(args);
}

TEST_F(BraveSyncHandlerUnittest, CopySyncCodeToClipboard) {
  base::ListValue args;
  constexpr char kSyncCodeExample[] = "the sync code";
  args.Append(base::Value("id"));
  args.Append(base::Value(kSyncCodeExample));
  CallHandleCopySyncCodeToClipboard(args);

  std::string ascii_text = ui::clipboard_test_util::ReadAsciiText(
      &clipboard(), ui::ClipboardBuffer::kCopyPaste,
      /* data_dst = */ nullptr);
  EXPECT_EQ(ascii_text, kSyncCodeExample);
  EXPECT_TRUE(clipboard().IsMarkedByOriginatorAsConfidential());
}
