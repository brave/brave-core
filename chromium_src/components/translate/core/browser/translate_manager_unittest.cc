/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../../components/translate/core/browser/translate_manager_unittest.cc" // NOLINT

namespace translate {

namespace testing {

TEST_F(TranslateManagerTest, CanManuallyTranslate_WithoutAPIKey) {
  TranslateManager::SetIgnoreMissingKeyForTesting(false);
  translate_manager_.reset(new translate::TranslateManager(
        &mock_translate_client_,
        &mock_translate_ranker_,
        &mock_language_model_));

  prefs_.SetBoolean(prefs::kOfferTranslateEnabled, true);
  ON_CALL(mock_translate_client_, IsTranslatableURL(GURL::EmptyGURL()))
          .WillByDefault(Return(true));
  network_notifier_.SimulateOnline();

  translate_manager_->GetLanguageState().LanguageDetermined("de", true);
  EXPECT_TRUE(translate_manager_->CanManuallyTranslate());
}

}  // namespace testing

}  // namespace translate
