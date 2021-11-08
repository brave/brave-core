/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/translate/core/browser/translate_manager.h"

#include <utility>

#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "build/build_config.h"
#include "components/language/core/browser/language_model.h"
#include "components/language/core/browser/language_prefs.h"
#include "components/language/core/common/language_experiments.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/translate/core/browser/mock_translate_client.h"
#include "components/translate/core/browser/mock_translate_driver.h"
#include "components/translate/core/browser/mock_translate_ranker.h"
#include "components/translate/core/browser/translate_accept_languages.h"
#include "components/translate/core/browser/translate_client.h"
#include "components/translate/core/browser/translate_download_manager.h"
#include "components/translate/core/browser/translate_pref_names.h"
#include "components/translate/core/browser/translate_prefs.h"
#include "components/translate/core/common/translate_constants.h"
#include "components/variations/variations_associated_data.h"
#include "google_apis/google_api_keys.h"
#include "net/base/mock_network_change_notifier.h"
#include "net/base/network_change_notifier.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Return;
using translate::TranslateDownloadManager;
using translate::TranslateLanguageList;
using translate::TranslateManager;
using translate::TranslatePrefs;

namespace {

// Overrides NetworkChangeNotifier, simulating connection type changes
// for tests.
// TODO(groby): Combine with similar code in ResourceRequestAllowedNotifierTest.
class TestNetworkChangeNotifier {
 public:
  TestNetworkChangeNotifier()
      : mock_notifier_(net::test::MockNetworkChangeNotifier::Create()) {}

  TestNetworkChangeNotifier(const TestNetworkChangeNotifier&) = delete;
  TestNetworkChangeNotifier& operator=(const TestNetworkChangeNotifier&) =
      delete;

  // Simulates a change of the connection type to |type|. This will notify any
  // objects that are NetworkChangeNotifiers.
  void SimulateNetworkConnectionChange(
      net::NetworkChangeNotifier::ConnectionType type) {
    mock_notifier_->SetConnectionType(type);
    net::NetworkChangeNotifier::NotifyObserversOfConnectionTypeChangeForTests(
        type);
    base::RunLoop().RunUntilIdle();
  }

  void SimulateOffline() {
    mock_notifier_->SetConnectionType(
        net::NetworkChangeNotifier::CONNECTION_NONE);
  }

  void SimulateOnline() {
    mock_notifier_->SetConnectionType(
        net::NetworkChangeNotifier::CONNECTION_UNKNOWN);
  }

 private:
  std::unique_ptr<net::test::MockNetworkChangeNotifier> mock_notifier_;
};

// A language model that just returns its instance variable.
class MockLanguageModel : public language::LanguageModel {
 public:
  explicit MockLanguageModel(const std::vector<LanguageDetails>& in_details)
      : details(in_details) {}

  std::vector<LanguageDetails> GetLanguages() override { return details; }

  std::vector<LanguageDetails> details;
};

}  // namespace

// The constructor of this class is used to register preferences before
// TranslatePrefs gets created.
struct ProfilePrefRegistration {
  explicit ProfilePrefRegistration(
      sync_preferences::TestingPrefServiceSyncable* prefs) {
    language::LanguagePrefs::RegisterProfilePrefs(prefs->registry());
    prefs->SetString(translate::testing::accept_languages_prefs, std::string());
#if defined(OS_CHROMEOS)
    prefs->SetString(
        translate::testing::preferred_languages_prefs, std::string());
#endif
    TranslatePrefs::RegisterProfilePrefs(prefs->registry());
    // TODO(groby): Figure out RegisterProfilePrefs() should register this.
    prefs->registry()->RegisterBooleanPref(
        translate::prefs::kOfferTranslateEnabled, true,
        user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  }
};

class TranslateManagerTest : public ::testing::Test {
 protected:
  TranslateManagerTest()
      : registration_(&prefs_),
        translate_prefs_(&prefs_),
        manager_(TranslateDownloadManager::GetInstance()),
        mock_translate_client_(&driver_, &prefs_),
        mock_language_model_({MockLanguageModel::LanguageDetails("en", 1.0)}) {}

  void SetUp() override {
    // Ensure we're not requesting a server-side translate language list.
    TranslateLanguageList::DisableUpdate();

    manager_->ResetForTesting();
  }

  void TearDown() override {
    manager_->ResetForTesting();
    variations::testing::ClearAllVariationParams();
  }

  // Utility function to prepare translate_manager_ for testing.
  void PrepareTranslateManager() {
    TranslateManager::SetIgnoreMissingKeyForTesting(true);
    translate_manager_ = std::make_unique<translate::TranslateManager>(
        &mock_translate_client_, &mock_translate_ranker_,
        &mock_language_model_);
  }

  void SetHasLanguageChanged(bool has_language_changed) {
    translate_manager_->GetLanguageState()->LanguageDetermined("de", true);
    translate_manager_->GetLanguageState()->DidNavigate(false, true, false,
                                                       std::string(), false);
    translate_manager_->GetLanguageState()->LanguageDetermined(
        has_language_changed ? "en" : "de", true);
    EXPECT_EQ(has_language_changed,
              translate_manager_->GetLanguageState()->HasLanguageChanged());
  }

  // Required to instantiate a net::test::MockNetworkChangeNotifier, because it
  // uses ObserverListThreadSafe.
  base::test::TaskEnvironment task_environment_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  ProfilePrefRegistration registration_;
  // TODO(groby): request TranslatePrefs from |mock_translate_client_| instead.
  TranslatePrefs translate_prefs_;
  TranslateDownloadManager* manager_;

  TestNetworkChangeNotifier network_notifier_;
  translate::testing::MockTranslateDriver driver_;
  translate::testing::MockTranslateRanker mock_translate_ranker_;
  ::testing::NiceMock<translate::testing::MockTranslateClient>
      mock_translate_client_;
  MockLanguageModel mock_language_model_;
  std::unique_ptr<TranslateManager> translate_manager_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(TranslateManagerTest, CanManuallyTranslate_WithoutAPIKey) {
  const std::string api_key = ::google_apis::GetAPIKey();
  ::google_apis::SetAPIKeyForTesting("dummytoken");
  EXPECT_FALSE(::google_apis::HasAPIKeyConfigured());

  TranslateManager::SetIgnoreMissingKeyForTesting(false);
  translate_manager_.reset(new translate::TranslateManager(
        &mock_translate_client_,
        &mock_translate_ranker_,
        &mock_language_model_));

  prefs_.SetBoolean(translate::prefs::kOfferTranslateEnabled, true);
  ON_CALL(mock_translate_client_, IsTranslatableURL(GURL::EmptyGURL()))
      .WillByDefault(Return(true));
  network_notifier_.SimulateOnline();

  translate_manager_->GetLanguageState()->LanguageDetermined("de", true);
  EXPECT_TRUE(translate_manager_->CanManuallyTranslate());

  ::google_apis::SetAPIKeyForTesting(api_key);
}

TEST_F(TranslateManagerTest, CanManuallyTranslate_WithAPIKey) {
  const std::string api_key = ::google_apis::GetAPIKey();
  ::google_apis::SetAPIKeyForTesting("notdummytoken");
  EXPECT_TRUE(::google_apis::HasAPIKeyConfigured());

  TranslateManager::SetIgnoreMissingKeyForTesting(false);
  translate_manager_.reset(new translate::TranslateManager(
        &mock_translate_client_,
        &mock_translate_ranker_,
        &mock_language_model_));

  prefs_.SetBoolean(translate::prefs::kOfferTranslateEnabled, true);
  ON_CALL(mock_translate_client_, IsTranslatableURL(GURL::EmptyGURL()))
      .WillByDefault(Return(true));
  network_notifier_.SimulateOnline();

  translate_manager_->GetLanguageState()->LanguageDetermined("de", true);
  EXPECT_TRUE(translate_manager_->CanManuallyTranslate());

  ::google_apis::SetAPIKeyForTesting(api_key);
}
