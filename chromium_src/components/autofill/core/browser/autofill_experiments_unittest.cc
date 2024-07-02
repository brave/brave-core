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
#include "components/sync/base/pref_names.h"
#include "components/sync/test/test_sync_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {

class AutofillExperimentsTest : public testing::Test {
 public:
  AutofillExperimentsTest() = default;

 protected:
  void SetUp() override {
    pref_service_.registry()->RegisterBooleanPref(
        syncer::prefs::internal::kSyncPayments, true);
    log_manager_ = LogManager::Create(nullptr, base::RepeatingClosure());
  }

  bool IsCreditCardUploadEnabled(
      const AutofillMetrics::PaymentsSigninState signin_state_for_metrics) {
    return IsCreditCardUploadEnabled("US", signin_state_for_metrics);
  }

  bool IsCreditCardUploadEnabled(
      const std::string& user_country,
      const AutofillMetrics::PaymentsSigninState signin_state_for_metrics) {
    return autofill::IsCreditCardUploadEnabled(&sync_service_, user_country,
                                               signin_state_for_metrics,
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
      AutofillMetrics::PaymentsSigninState::kSignedInAndSyncFeatureEnabled));
}

TEST_F(AutofillExperimentsTest, IsCardUploadEnabled_FeatureDisabled) {
  scoped_feature_list_.InitAndDisableFeature(features::kAutofillUpstream);
  EXPECT_FALSE(IsCreditCardUploadEnabled(
      AutofillMetrics::PaymentsSigninState::kSignedInAndSyncFeatureEnabled));
}

TEST_F(
    AutofillExperimentsTest,
    IsCardUploadEnabled_TransportSyncDoesNotHaveAutofillProfileActiveDataType) {
  scoped_feature_list_.InitWithFeatures(
      /*enabled_features=*/{features::kAutofillUpstream},
      /*disabled_features=*/{});
  // When we have no primary account, Sync will start in Transport-only mode
  // (if allowed).
  sync_service_.SetAllowedByEnterprisePolicy(false);

  // Update the active types to only include Wallet. This disables all other
  // types, including profiles.
  sync_service_.GetUserSettings()->SetSelectedTypes(
      /*sync_everything=*/false,
      /*types=*/syncer::UserSelectableTypeSet(
          {syncer::UserSelectableType::kAutofill}));
  sync_service_.SetFailedDataTypes({syncer::AUTOFILL_PROFILE});

  EXPECT_FALSE(IsCreditCardUploadEnabled(
      AutofillMetrics::PaymentsSigninState::kSignedInAndSyncFeatureEnabled));
}

TEST_F(AutofillExperimentsTest, IsCardUploadEnabled_UserEmailWithGoogleDomain) {
  scoped_feature_list_.InitAndEnableFeature(features::kAutofillUpstream);
  EXPECT_FALSE(IsCreditCardUploadEnabled(
      AutofillMetrics::PaymentsSigninState::kSignedInAndSyncFeatureEnabled));
}

TEST_F(AutofillExperimentsTest,
       IsCardUploadEnabled_UserEmailWithNonGoogleDomainIfExperimentEnabled) {
  scoped_feature_list_.InitWithFeatures({features::kAutofillUpstream}, {});
  EXPECT_FALSE(IsCreditCardUploadEnabled(
      AutofillMetrics::PaymentsSigninState::kSignedInAndSyncFeatureEnabled));
}

}  // namespace autofill
