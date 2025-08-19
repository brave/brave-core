/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_sync_handler.h"

#include <memory>
#include <string>

#include "base/values.h"
#include "build/build_config.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_ui.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/clipboard/clipboard.h"

class BraveSyncHandlerUnittest : public testing::Test {
 public:
  BraveSyncHandlerUnittest() {
    TestingProfile::Builder builder;

    profile_ = builder.Build();
    web_contents_ = content::WebContents::Create(
        content::WebContents::CreateParams(profile_.get()));

    test_web_ui_.set_web_contents(web_contents_.get());

    handler_ = std::make_unique<BraveSyncHandler>(/*profile_.get()*/);
    handler_->set_web_ui(&test_web_ui_);
    handler_->RegisterMessages();
  }
  ~BraveSyncHandlerUnittest() override {
    // The test handler unusually owns its own TestWebUI, so we make sure to
    // unbind it from the base class before the derived class is destroyed.
    handler_->set_web_ui(nullptr);
    handler_.reset();
  }

 protected:
  content::TestWebUI* web_ui() { return &test_web_ui_; }
  BraveSyncHandler* handler() { return handler_.get(); }

 private:
  std::unique_ptr<BraveSyncHandler> handler_;
  content::BrowserTaskEnvironment browser_task_environment;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<content::WebContents> web_contents_;
  content::TestWebUI test_web_ui_;
};

TEST_F(BraveSyncHandlerUnittest, CopySyncCodeToClipboard) {
  base::Value::List args;
  constexpr char kSyncCodeExample[] = "the sync code";
  args.Append(base::Value("id"));
  args.Append(base::Value(kSyncCodeExample));
  handler()->HandleCopySyncCodeToClipboard(args);

  std::string ascii_text;
  ui::Clipboard::GetForCurrentThread()->ReadAsciiText(
      ui::ClipboardBuffer::kCopyPaste,
      /* data_dst = */ nullptr, &ascii_text);
  EXPECT_EQ(ascii_text, kSyncCodeExample);

#if BUILDFLAG(IS_MAC)
  // IsMarkedByOriginatorAsConfidential is implemented for mac only
  EXPECT_TRUE(ui::Clipboard::GetForCurrentThread()
                  ->IsMarkedByOriginatorAsConfidential());
#endif  // BUILDFLAG(IS_MAC)
}
