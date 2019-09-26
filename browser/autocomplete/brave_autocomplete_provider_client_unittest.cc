/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/autocomplete/brave_autocomplete_provider_client.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/fake_service_worker_context.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveAutocompleteProviderClientUnitTest : public testing::Test {
 public:
  void SetUp() override {
    profile_ = CreateProfile();
    client_ =
        std::make_unique<BraveAutocompleteProviderClient>(profile_.get());
  }

  bool BuiltinExists(const base::string16& builtin) {
    std::vector<base::string16> v = client_->GetBuiltinURLs();
    auto it = std::find(v.begin(), v.end(), builtin);
    return it != v.end();
  }

 private:
  std::unique_ptr<TestingProfile> CreateProfile() {
    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    return builder.Build();
  }

  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<BraveAutocompleteProviderClient> client_;
};

TEST_F(BraveAutocompleteProviderClientUnitTest,
       SyncURLSuggestedNotSyncInternal) {
  ASSERT_FALSE(
      BuiltinExists(base::ASCIIToUTF16(chrome::kChromeUISyncInternalsHost)));
  ASSERT_TRUE(BuiltinExists(base::ASCIIToUTF16(kBraveUISyncHost)));
}
