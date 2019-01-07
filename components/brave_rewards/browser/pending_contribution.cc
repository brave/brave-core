/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/browser/pending_contribution.h"

namespace brave_rewards {

  PendingContribution::PendingContribution() :
    amount(0),
    added_date(0),
    reconcile_date(0) {
  }

  PendingContribution::~PendingContribution() { }

  PendingContribution::PendingContribution(const PendingContribution &data) {
    publisher_key = data.publisher_key;
    amount = data.amount;
    added_date = data.added_date;
    reconcile_date = data.reconcile_date;
  }

}  // namespace brave_rewards
