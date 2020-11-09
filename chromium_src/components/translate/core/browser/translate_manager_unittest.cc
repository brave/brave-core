/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../../components/translate/core/browser/translate_manager_unittest.cc" // NOLINT
#include "google_apis/google_api_keys.h"

namespace translate {

namespace testing {

TEST_F(TranslateManagerTest, CanManuallyTranslate_WithoutAPIKey) {
  const std::string api_key = ::google_apis::GetAPIKey();
  ::google_apis::SetAPIKeyForTesting("dummytoken");
  EXPECT_FALSE(::google_apis::HasAPIKeyConfigured());

  TranslateManager::SetIgnoreMissingKeyForTesting(false);
  translate_manager_.reset(new translate::TranslateManager(
        &mock_translate_client_,
        &mock_translate_ranker_,
        &mock_language_model_));

  prefs_.SetBoolean(prefs::kOfferTranslateEnabled, true);
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

  prefs_.SetBoolean(prefs::kOfferTranslateEnabled, true);
  ON_CALL(mock_translate_client_, IsTranslatableURL(GURL::EmptyGURL()))
          .WillByDefault(Return(true));
  network_notifier_.SimulateOnline();

  translate_manager_->GetLanguageState()->LanguageDetermined("de", true);
  EXPECT_TRUE(translate_manager_->CanManuallyTranslate());

  ::google_apis::SetAPIKeyForTesting(api_key);
}

}  // namespace testing

}  // namespace translate
