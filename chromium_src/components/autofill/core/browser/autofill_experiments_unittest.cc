/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/autofill/core/browser/autofill_experiments.h"

#include "base/test/scoped_feature_list.h"
#include "components/autofill/core/browser/logging/log_manager.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/autofill/core/common/autofill_payments_features.h"
#include "components/autofill/core/common/autofill_prefs.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync/driver/test_sync_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {

class AutofillExperimentsTest : public testing::Test {
 public:
  AutofillExperimentsTest() {}

 protected:
  void SetUp() override {
    pref_service_.registry()->RegisterBooleanPref(
        prefs::kAutofillWalletImportEnabled, true);
    log_manager_ = LogManager::Create(nullptr, base::RepeatingClosure());
  }

  bool IsCreditCardUploadEnabled(const AutofillSyncSigninState sync_state) {
    return IsCreditCardUploadEnabled("john.smith@gmail.com", sync_state);
  }

  bool IsCreditCardUploadEnabled(const std::string& user_email,
                                 const AutofillSyncSigninState sync_state) {
    return autofill::IsCreditCardUploadEnabled(&pref_service_, &sync_service_,
                                               user_email, sync_state,
                                               log_manager_.get());
  }

  base::test::ScopedFeatureList scoped_feature_list_;
  TestingPrefServiceSimple pref_service_;
  syncer::TestSyncService sync_service_;
  std::unique_ptr<LogManager> log_manager_;
};

TEST_F(AutofillExperimentsTest, IsCardUploadEnabled_FeatureEnabled) {
  scoped_feature_list_.InitAndEnableFeature(features::kAutofillUpstream);
  EXPECT_FALSE(IsCreditCardUploadEnabled(
      AutofillSyncSigninState::kSignedInAndSyncFeatureEnabled));
}

TEST_F(AutofillExperimentsTest, IsCardUploadEnabled_FeatureDisabled) {
  scoped_feature_list_.InitAndDisableFeature(features::kAutofillUpstream);
  EXPECT_FALSE(IsCreditCardUploadEnabled(
      AutofillSyncSigninState::kSignedInAndSyncFeatureEnabled));
}

TEST_F(
    AutofillExperimentsTest,
    IsCardUploadEnabled_TransportSyncDoesNotHaveAutofillProfileActiveDataType) {
  scoped_feature_list_.InitWithFeatures(
      /*enable_features=*/{features::kAutofillUpstream,
                           features::kAutofillEnableAccountWalletStorage},
      /*disable_features=*/{});
  // When we have no primary account, Sync will start in Transport-only mode
  // (if allowed).
  sync_service_.SetIsAuthenticatedAccountPrimary(false);

  // Update the active types to only include Wallet. This disables all other
  // types, including profiles.
  sync_service_.SetActiveDataTypes(
      syncer::ModelTypeSet(syncer::AUTOFILL_WALLET_DATA));

  EXPECT_FALSE(IsCreditCardUploadEnabled(
      AutofillSyncSigninState::kSignedInAndSyncFeatureEnabled));
}

TEST_F(AutofillExperimentsTest, IsCardUploadEnabled_UserEmailWithGoogleDomain) {
  scoped_feature_list_.InitAndEnableFeature(features::kAutofillUpstream);
  EXPECT_FALSE(IsCreditCardUploadEnabled(
      "john.smith@gmail.com",
      AutofillSyncSigninState::kSignedInAndSyncFeatureEnabled));
  EXPECT_FALSE(IsCreditCardUploadEnabled(
      "googler@google.com",
      AutofillSyncSigninState::kSignedInAndSyncFeatureEnabled));
  EXPECT_FALSE(IsCreditCardUploadEnabled(
      "old.school@googlemail.com",
      AutofillSyncSigninState::kSignedInAndSyncFeatureEnabled));
  EXPECT_FALSE(IsCreditCardUploadEnabled(
      "code.committer@chromium.org",
      AutofillSyncSigninState::kSignedInAndSyncFeatureEnabled));
}

TEST_F(AutofillExperimentsTest,
       IsCardUploadEnabled_UserEmailWithNonGoogleDomainIfExperimentEnabled) {
  scoped_feature_list_.InitWithFeatures(
      {features::kAutofillUpstream,
       features::kAutofillUpstreamAllowAllEmailDomains},
      {});
  EXPECT_FALSE(IsCreditCardUploadEnabled(
      "cool.user@hotmail.com",
      AutofillSyncSigninState::kSignedInAndSyncFeatureEnabled));
  EXPECT_FALSE(IsCreditCardUploadEnabled(
      "john.smith@johnsmith.com",
      AutofillSyncSigninState::kSignedInAndSyncFeatureEnabled));
  EXPECT_FALSE(IsCreditCardUploadEnabled(
      "fake.googler@google.net",
      AutofillSyncSigninState::kSignedInAndSyncFeatureEnabled));
  EXPECT_FALSE(IsCreditCardUploadEnabled(
      "fake.committer@chromium.com",
      AutofillSyncSigninState::kSignedInAndSyncFeatureEnabled));
}

}  // namespace autofill
