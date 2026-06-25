/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "components/sync/base/data_type.h"
#include "components/sync/base/user_selectable_type.h"
#include "components/sync/service/sync_service.h"
#include "components/sync/service/sync_user_settings.h"
#include "content/public/test/browser_test.h"

namespace ai_chat {
namespace {

class AIChatSyncBrowserTest : public InProcessBrowserTest {
 public:
  AIChatSyncBrowserTest() {
    features_.InitWithFeatures(
        /*enabled_features=*/{features::kBraveSyncAIChat,
                              features::kAIChatHistory},
        /*disabled_features=*/{});
  }

  AIChatSyncBrowserTest(const AIChatSyncBrowserTest&) = delete;
  AIChatSyncBrowserTest& operator=(const AIChatSyncBrowserTest&) = delete;
  ~AIChatSyncBrowserTest() override = default;

 private:
  base::test::ScopedFeatureList features_;
};

// Verifies that AI_CHAT_CONVERSATION is registered as a sync data type and
// surfaced as the kAIChat selectable type when the feature is enabled and on-
// disk storage is on. This exercises the registration chain:
//
//   SyncServiceFactory  →  CommonControllerBuilder::SetAIChatService(...)
//   →  CommonControllerBuilder::Build()  →  BRAVE_BUILD_SYNC_CONTROLLERS
//   →  AIChatService::CreateSyncControllerDelegate()
//
// If CreateSyncControllerDelegate() returns nullptr (because the bridge
// hasn't been created yet on the DB sequence), the AI Chat controller is
// never added and kAIChat will be absent from the registered selectable
// types — which is what hides the desktop checkbox via
// `hidden="[[!syncPrefs.aiChatRegistered]]"`.
IN_PROC_BROWSER_TEST_F(AIChatSyncBrowserTest,
                       AIChatConversationDataTypeIsRegistered) {
  // The bridge is only created when on-disk storage is enabled.
  Profile* profile = browser()->profile();
  profile->GetPrefs()->SetBoolean(prefs::kBraveChatStorageEnabled, true);

  syncer::SyncService* sync_service =
      SyncServiceFactory::GetForProfile(profile);
  ASSERT_TRUE(sync_service);

  EXPECT_TRUE(
      sync_service->GetUserSettings()->GetRegisteredSelectableTypes().Has(
          syncer::UserSelectableType::kAIChat));
}

}  // namespace
}  // namespace ai_chat
