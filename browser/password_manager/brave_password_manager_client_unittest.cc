/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/functional/bind.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/password_manager/chrome_password_manager_client.h"
#include "chrome/browser/password_manager/factories/password_manager_settings_service_factory.h"
#include "chrome/browser/password_manager/factories/profile_password_store_factory.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "chrome/browser/ui/autofill/chrome_autofill_client.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "components/autofill/core/common/autofill_test_util.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/password_manager/core/browser/mock_password_manager_settings_service.h"
#include "components/password_manager/core/browser/password_manager_setting.h"
#include "components/password_manager/core/browser/password_manager_test_utils.h"
#include "components/password_manager/core/browser/password_store/mock_password_store_interface.h"
#include "components/prefs/pref_service.h"
#include "components/sync/test/test_sync_service.h"
#include "content/public/browser/browser_context.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

using password_manager::MockPasswordManagerSettingsService;
using password_manager::PasswordManagerSetting;
using testing::NiceMock;
using testing::Return;

std::unique_ptr<KeyedService> BuildTestSyncService(
    content::BrowserContext* context) {
  return std::make_unique<syncer::TestSyncService>();
}

std::unique_ptr<KeyedService> BuildSettingsServiceOfferingToSavePasswords(
    content::BrowserContext* context) {
  auto settings_service =
      std::make_unique<NiceMock<MockPasswordManagerSettingsService>>();
  ON_CALL(*settings_service,
          IsSettingEnabled(PasswordManagerSetting::kOfferToSavePasswords))
      .WillByDefault(Return(true));
  return settings_service;
}

}  // namespace

// Tests that ChromePasswordManagerClient::IsFillingEnabled() and
// IsSavingAndFillingEnabled() honor Brave's "Autofill passwords using Brave"
// setting (kBravePasswordManagerFillEnabled).
// See https://github.com/brave/brave-browser/issues/19065.
class BravePasswordManagerClientTest : public ChromeRenderViewHostTestHarness {
 protected:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    // Meet all of upstream's conditions for filling and saving, so that the
    // Brave setting is the only thing that can turn them off.
    ProfilePasswordStoreFactory::GetInstance()->SetTestingFactory(
        GetBrowserContext(),
        base::BindRepeating(&password_manager::BuildPasswordStoreInterface<
                            content::BrowserContext,
                            password_manager::MockPasswordStoreInterface>));
    PasswordManagerSettingsServiceFactory::GetInstance()->SetTestingFactory(
        profile(),
        base::BindRepeating(&BuildSettingsServiceOfferingToSavePasswords));
    // The sync service must be set up before the client is created.
    SyncServiceFactory::GetInstance()->SetTestingFactoryAndUse(
        profile(), base::BindRepeating(&BuildTestSyncService));

    // ChromePasswordManagerClient observes the AutofillManagers owned by
    // ChromeAutofillClient, so that has to be created first.
    autofill::ChromeAutofillClient::CreateForWebContents(web_contents());
    ChromePasswordManagerClient::CreateForWebContents(web_contents());
  }

  ChromePasswordManagerClient* client() {
    return ChromePasswordManagerClient::FromWebContents(web_contents());
  }

  PrefService* prefs() { return profile()->GetPrefs(); }

  url::Origin origin() const {
    return url::Origin::Create(GURL("https://example.com"));
  }

 private:
  autofill::test::AutofillUnitTestEnvironment autofill_environment_{
      {.disable_server_communication = true}};
};

TEST_F(BravePasswordManagerClientTest, FillingAndSavingEnabledByDefault) {
  EXPECT_TRUE(prefs()->GetBoolean(kBravePasswordManagerFillEnabled));
  EXPECT_TRUE(client()->IsFillingEnabled(origin()));
  EXPECT_TRUE(client()->IsSavingAndFillingEnabled(origin()));
}

TEST_F(BravePasswordManagerClientTest, SettingDisablesFillingAndSaving) {
  prefs()->SetBoolean(kBravePasswordManagerFillEnabled, false);
  EXPECT_FALSE(client()->IsFillingEnabled(origin()));
  EXPECT_FALSE(client()->IsSavingAndFillingEnabled(origin()));

  // Turning the setting back on restores upstream behavior.
  prefs()->SetBoolean(kBravePasswordManagerFillEnabled, true);
  EXPECT_TRUE(client()->IsFillingEnabled(origin()));
  EXPECT_TRUE(client()->IsSavingAndFillingEnabled(origin()));
}
