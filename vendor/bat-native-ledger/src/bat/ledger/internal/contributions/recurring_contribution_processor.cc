/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contributions/recurring_contribution_processor.h"

#include <algorithm>
#include <string>
#include <vector>

#include "bat/ledger/internal/contributions/contribution_router.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/delay_generator.h"
#include "bat/ledger/internal/core/job_store.h"
#include "bat/ledger/internal/core/value_converters.h"

namespace ledger {

namespace {

struct RecurringContributionState {
  std::string publisher_id;
  double amount = 0;
  bool completed = false;

  auto ToValue() const {
    ValueWriter w;
    w.Write("publisher_id", publisher_id);
    w.Write("amount", amount);
    w.Write("completed", completed);
    return w.Finish();
  }

  static auto FromValue(const base::Value& value) {
    StructValueReader<RecurringContributionState> r(value);
    r.Read("publisher_id", &RecurringContributionState::publisher_id);
    r.Read("amount", &RecurringContributionState::amount);
    r.Read("completed", &RecurringContributionState::completed);
    return r.Finish();
  }
};

struct ProcessorState {
  std::vector<RecurringContributionState> contributions;

  auto ToValue() const {
    ValueWriter w;
    w.Write("contributions", contributions);
    return w.Finish();
  }

  static auto FromValue(const base::Value& value) {
    StructValueReader<ProcessorState> r(value);
    r.Read("contributions", &ProcessorState::contributions);
    return r.Finish();
  }
};

class ProcessJob : public ResumableJob<bool, ProcessorState> {
 public:
  static constexpr char kJobType[] = "recurring-contribution";

 protected:
  void Resume() override {
    contribution_iter_ = state().contributions.begin();
    SendNext();
  }

  void OnStateInvalid() override { Complete(false); }

 private:
  void SendNext() {
    contribution_iter_ =
        std::find_if(contribution_iter_, state().contributions.end(),
                     [](const RecurringContributionState& contribution) {
                       return !contribution.completed;
                     });

    if (contribution_iter_ == state().contributions.end()) {
      context().LogVerbose(FROM_HERE) << "Recurring contributions completed";
      return Complete(true);
    }

    context()
        .Get<DelayGenerator>()
        .RandomDelay(FROM_HERE, kBackgroundContributionDelay)
        .Then(ContinueWith(this, &ProcessJob::OnSendNextDelayElapsed));
  }

  void OnSendNextDelayElapsed(base::TimeDelta) {
    DCHECK(contribution_iter_ != state().contributions.end());
    context()
        .Get<ContributionRouter>()
        .SendContribution(ContributionType::kRecurring,
                          contribution_iter_->publisher_id,
                          contribution_iter_->amount)
        .Then(ContinueWith(this, &ProcessJob::OnContributionSent));
  }

  void OnContributionSent(bool success) {
    DCHECK(contribution_iter_ != state().contributions.end());

    if (!success) {
      // If we are unable to send this contribution for any reason, assume that
      // the failure is unrecoverable (e.g. the publisher is not registered or
      // verified with a matching wallet provider) and continue on with the next
      // recurring contribution.
      context().LogError(FROM_HERE) << "Unable to send recurring contribution";
    }

    contribution_iter_->completed = true;
    SaveState();
    SendNext();
  }

  std::vector<RecurringContributionState>::iterator contribution_iter_;
};

}  // namespace

std::string RecurringContributionProcessor::StartContributions(
    const std::vector<RecurringContribution>& contributions) {
  ProcessorState state;
  for (auto& contribution : contributions) {
    state.contributions.push_back({.publisher_id = contribution.publisher_id,
                                   .amount = contribution.amount});
  }
  return context().Get<JobStore>().InitializeJobState<ProcessJob>(state);
}

Future<bool> RecurringContributionProcessor::ResumeContributions(
    const std::string& job_id) {
  return context().StartJob<ProcessJob>(job_id);
}

}  // namespace ledger
