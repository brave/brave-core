/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_STATE_STATE_KEYS_H_
#define BRAVELEDGER_STATE_STATE_KEYS_H_

#include <string>

namespace ledger {
  const char kStateEnabled[] = "enabled";
  const char kStateServerPublisherListStamp[] = "server_publisher_list_stamp";
  const char kStateUpholdAnonAddress[] = "uphold_anon_address";
  const char kStatePromotionLastFetchStamp[] = "promotion_last_fetch_stamp";
  const char kStatePromotionCorruptedMigrated[] =
      "promotion_corrupted_migrated2";
  const char kStateAnonTransferChecked[] = "anon_transfer_checked";
  const char kStateVersion[] = "version";

  // Auto contributions
  const char kStateMinVisitTime[] = "ac.min_visit_time";
  const char kStateMinVisits[] = "ac.min_visits";
  const char kStateAllowNonVerified[] = "ac.allow_non_verified";
  const char kStateAllowVideoContribution[] = "ac.allow_video_contributions";
  const char kStateScoreA[] = "ac.score.a";
  const char kStateScoreB[] = "ac.score.b";
  const char kStateAutoContributeEnabled[] = "ac.enabled";
  const char kStateAutoContributeAmount[] = "ac.amount";
  const char kStateNextReconcileStamp[] = "ac.next_reconcile_stamp";
  const char kStateCreationStamp[] = "creation_stamp";
  const char kStateAnonymousCardId[] = "wallet.anonymous_card_id";
  const char kStateRecoverySeed[] = "wallet.seed";
  const char kStatePaymentId[] = "wallet.payment_id";
  const char kStateInlineTipRedditEnabled[] = "inline_tip.reddit";
  const char kStateInlineTipTwitterEnabled[] = "inline_tip.twitter";
  const char kStateInlineTipGithubEnabled[] = "inline_tip.github";
  const char kStateParametersRate[] = "parameters.rate";
  const char kStateParametersAutoContributeChoice[] = "parameters.ac.choice";
  const char kStateParametersAutoContributeChoices[] = "parameters.ac.choices";
  const char kStateParametersTipChoices[] = "parameters.tip.choices";
  const char kStateParametersMonthlyTipChoices[] =
      "parameters.tip.monthly_choices";
  const char kStateFetchOldBalance[] = "fetch_old_balance";

}  // namespace ledger

#endif  // BRAVELEDGER_STATE_STATE_KEYS_H_
