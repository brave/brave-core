// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/web_ui_mocha_browser_test.h"
#include "content/public/test/browser_test.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "base/test/scoped_feature_list.h"
#include "brave/components/ai_chat/core/common/features.h"

// Mocha browser tests for Brave's modifications to the desktop settings WebUI.
// Runs in the brave_browser_tests binary so it is exercised by Brave CI.
class BraveSettingsBrowserTest : public WebUIMochaBrowserTest {
 protected:
  BraveSettingsBrowserTest() {
    // The AI Chat sync toggle is injected into settings-sync-controls only when
    // this feature is enabled (surfaced as `isBraveSyncAIChatEnabled`).
    scoped_feature_list_.InitAndEnableFeature(
        ai_chat::features::kBraveSyncAIChat);
    set_test_loader_host(chrome::kChromeUISettingsHost);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

using BraveSettingsTest = BraveSettingsBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveSettingsTest, SyncControls) {
  RunTest("brave_settings/brave_sync_controls_test.js", "mocha.run()");
}
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
