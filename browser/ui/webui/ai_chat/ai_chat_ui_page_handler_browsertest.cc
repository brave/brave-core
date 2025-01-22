// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"

#include "base/test/gtest_util.h"
#include "base/test/run_until.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

namespace ai_chat {
class AiChatUiPageHandlerBrowserTest : public InProcessBrowserTest {
 public:
  AiChatUiPageHandlerBrowserTest() = default;
  ~AiChatUiPageHandlerBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // TODO: Mock a WebUI page?
    // TODO: ???:
  }
};

IN_PROC_BROWSER_TEST_F(AiChatUiPageHandlerBrowserTest, WebContentsIsLoadedIfNeeded) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  web_contents->GetController().SetNeedsReload();
}

}  // namespace ai_chat
