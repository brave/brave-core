/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_sync/test/test_sync_code.h"
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "chrome/browser/autofill/personal_data_manager_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/autofill/core/browser/data_manager/payments/payments_data_manager.h"
#include "components/autofill/core/browser/data_manager/personal_data_manager.h"
#include "components/prefs/pref_service.h"
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

// With AI Chat sync enabled, AI_CHAT_CONVERSATION is registered, so the user
// can select it as kAIChat.
IN_PROC_BROWSER_TEST_F(AIChatSyncBrowserTest,
                       AIChatConversationDataTypeIsRegistered) {
  // Chat history storage is on by default.
  // DataTypeIsRegisteredWithStorageOffAtStartup covers it off.
  Profile* profile = GetProfile();
  ASSERT_TRUE(profile->GetPrefs()->GetBoolean(prefs::kBraveChatStorageEnabled));

  syncer::SyncService* sync_service =
      SyncServiceFactory::GetForProfile(profile);
  ASSERT_TRUE(sync_service);

  EXPECT_TRUE(
      sync_service->GetUserSettings()->GetRegisteredSelectableTypes().Has(
          syncer::UserSelectableType::kAIChat));
}

// Registration doesn't depend on the chat history storage pref (see
// AIChatDataTypeController). The PRE_ test turns storage off and exits; the
// browser then restarts with the same profile, so the pref is already off when
// the SyncService is built.
IN_PROC_BROWSER_TEST_F(AIChatSyncBrowserTest,
                       PRE_DataTypeIsRegisteredWithStorageOffAtStartup) {
  GetProfile()->GetPrefs()->SetBoolean(prefs::kBraveChatStorageEnabled, false);
}

IN_PROC_BROWSER_TEST_F(AIChatSyncBrowserTest,
                       DataTypeIsRegisteredWithStorageOffAtStartup) {
  Profile* profile = GetProfile();
  ASSERT_FALSE(
      profile->GetPrefs()->GetBoolean(prefs::kBraveChatStorageEnabled));

  syncer::SyncService* sync_service =
      SyncServiceFactory::GetForProfile(profile);
  ASSERT_TRUE(sync_service);

  EXPECT_TRUE(
      sync_service->GetUserSettings()->GetRegisteredSelectableTypes().Has(
          syncer::UserSelectableType::kAIChat));
}

// SyncServiceFactory creates the AI Chat sync data type controller while it
// builds the SyncService, and the controller must not look up AIChatService
// then. AIChatService depends, through ProfileMiscMetricsService, on
// PersonalDataManager, which needs the SyncService: a lookup during the build
// would leave PersonalDataManager without one. PaymentsDataManager observes
// the SyncService that PersonalDataManager is given.
IN_PROC_BROWSER_TEST_F(AIChatSyncBrowserTest,
                       PersonalDataManagerGetsSyncService) {
  Profile* profile = GetProfile();
  syncer::SyncService* sync_service =
      SyncServiceFactory::GetForProfile(profile);
  ASSERT_TRUE(sync_service);
  autofill::PersonalDataManager* personal_data_manager =
      autofill::PersonalDataManagerFactory::GetForBrowserContext(profile);
  ASSERT_TRUE(personal_data_manager);

  EXPECT_TRUE(sync_service->HasObserver(
      &personal_data_manager->payments_data_manager()));
}

class AIChatSyncWithoutAIChatBrowserTest : public InProcessBrowserTest {
 public:
  AIChatSyncWithoutAIChatBrowserTest() {
    features_.InitWithFeatures(
        /*enabled_features=*/{features::kBraveSyncAIChat},
        /*disabled_features=*/{features::kAIChat});
  }

  AIChatSyncWithoutAIChatBrowserTest(
      const AIChatSyncWithoutAIChatBrowserTest&) = delete;
  AIChatSyncWithoutAIChatBrowserTest& operator=(
      const AIChatSyncWithoutAIChatBrowserTest&) = delete;
  ~AIChatSyncWithoutAIChatBrowserTest() override = default;

 private:
  base::test::ScopedFeatureList features_;
};

// The AI Chat sync data type's precondition reads the chat history storage
// pref, which is registered only while AI Chat is enabled, so with AI Chat off
// the type isn't registered, even with AI Chat sync on. Joining a sync chain
// then mustn't select it either. SyncUserSettings::SetSelectedTypes() DCHECKs
// that it's given only registered types, and GetSelectedTypes() lists only
// registered ones, so the join completing is the check.
IN_PROC_BROWSER_TEST_F(AIChatSyncWithoutAIChatBrowserTest,
                       DataTypeIsNotRegistered) {
  Profile* profile = GetProfile();
  ASSERT_FALSE(
      profile->GetPrefs()->FindPreference(prefs::kBraveChatStorageEnabled));

  auto* sync_service = static_cast<syncer::BraveSyncServiceImpl*>(
      SyncServiceFactory::GetAsSyncServiceImplForProfileForTesting(profile));
  ASSERT_TRUE(sync_service);
  EXPECT_FALSE(
      sync_service->GetUserSettings()->GetRegisteredSelectableTypes().Has(
          syncer::UserSelectableType::kAIChat));

  ASSERT_TRUE(sync_service->SetSyncCode(brave_sync::kValidSyncCode));
  EXPECT_TRUE(base::test::RunUntil([&] {
    return sync_service->GetUserSettings()->GetSelectedTypes().Has(
        syncer::UserSelectableType::kBookmarks);
  }));
}

}  // namespace
}  // namespace ai_chat
