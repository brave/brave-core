/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contributions/pending_contribution_processor.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "bat/ledger/internal/contributions/contribution_router.h"
#include "bat/ledger/internal/contributions/contribution_store.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/delay_generator.h"
#include "bat/ledger/internal/core/job_store.h"
#include "bat/ledger/internal/core/value_converters.h"

namespace ledger {

namespace {

constexpr base::TimeDelta kPendingExpiresAfter = base::Days(90);

enum class ProcessorStatus { kPending, kSending, kComplete };

std::string StringifyEnum(ProcessorStatus status) {
  switch (status) {
    case ProcessorStatus::kPending:
      return "pending";
    case ProcessorStatus::kSending:
      return "sending";
    case ProcessorStatus::kComplete:
      return "complete";
  }
}

absl::optional<ProcessorStatus> ParseEnum(
    const EnumString<ProcessorStatus>& s) {
  return s.Match({ProcessorStatus::kPending, ProcessorStatus::kSending,
                  ProcessorStatus::kComplete});
}

struct PendingContributionState {
  std::string publisher_id;
  double amount = 0;
  base::Time created_at;
  bool completed = false;

  auto ToValue() const {
    ValueWriter w;
    w.Write("publisher_id", publisher_id);
    w.Write("amount", amount);
    w.Write("created_at", created_at);
    w.Write("completed", completed);
    return w.Finish();
  }

  static auto FromValue(const base::Value& value) {
    StructValueReader<PendingContributionState> r(value);
    r.Read("publisher_id", &PendingContributionState::publisher_id);
    r.Read("amount", &PendingContributionState::amount);
    r.Read("created_at", &PendingContributionState::created_at);
    r.Read("completed", &PendingContributionState::completed);
    return r.Finish();
  }
};

struct ProcessorState {
  std::vector<PendingContributionState> contributions;
  ProcessorStatus status = ProcessorStatus::kPending;

  auto ToValue() const {
    ValueWriter w;
    w.Write("contributions", contributions);
    w.Write("status", status);
    return w.Finish();
  }

  static auto FromValue(const base::Value& value) {
    StructValueReader<ProcessorState> r(value);
    r.Read("contributions", &ProcessorState::contributions);
    r.Read("status", &ProcessorState::status);
    return r.Finish();
  }
};

class ProcessJob : public ResumableJob<bool, ProcessorState> {
 public:
  static constexpr char kJobType[] = "pending-contribution";

 protected:
  void Resume() override {
    contribution_iter_ = state().contributions.begin();

    switch (state().status) {
      case ProcessorStatus::kPending:
        return LoadContributions();
      case ProcessorStatus::kSending:
        return SendNext();
      case ProcessorStatus::kComplete:
        return Complete(true);
    }
  }

  void OnStateInvalid() override { Complete(false); }

 private:
  void LoadContributions() {
    context().LogVerbose(FROM_HERE) << "Sending pending contributions";
    context().Get<ContributionStore>().GetPendingContributions().Then(
        ContinueWith(this, &ProcessJob::OnContributionsLoaded));
  }

  void OnContributionsLoaded(std::vector<PendingContribution> contributions) {
    DCHECK(state().contributions.empty());

    for (auto& contribution : contributions) {
      state().contributions.push_back(
          {.publisher_id = std::move(contribution.publisher_id),
           .amount = contribution.amount,
           .created_at = contribution.created_at});
    }

    context().Get<ContributionStore>().ClearPendingContributions().Then(
        ContinueWith(this, &ProcessJob::OnContributionsCleared));
  }

  void OnContributionsCleared(bool success) {
    if (!success) {
      context().LogError(FROM_HERE) << "Unable to clear pending contributions";
      return Complete(false);
    }

    contribution_iter_ = state().contributions.begin();

    state().status = ProcessorStatus::kSending;
    SaveState();
    SendNext();
  }

  void SendNext() {
    contribution_iter_ =
        std::find_if(contribution_iter_, state().contributions.end(),
                     [](const PendingContributionState& contribution) {
                       return !contribution.completed;
                     });

    if (contribution_iter_ == state().contributions.end()) {
      context().LogVerbose(FROM_HERE) << "Pending contributions completed";
      state().status = ProcessorStatus::kComplete;
      SaveState();
      return Complete(true);
    }

    base::Time expires_at =
        contribution_iter_->created_at + kPendingExpiresAfter;

    if (base::Time::Now() >= expires_at) {
      return OnContributionSent(true);
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
        .SendContribution(ContributionType::kOneTime,
                          contribution_iter_->publisher_id,
                          contribution_iter_->amount)
        .Then(ContinueWith(this, &ProcessJob::OnContributionSent));
  }

  void OnContributionSent(bool success) {
    DCHECK(contribution_iter_ != state().contributions.end());

    if (!success) {
      // If the contribution could not be sent for any reason (although usually
      // because the publisher is not yet configured to accept contibutions from
      // the user), then add the pending contribution back to the database.
      context().Get<ContributionStore>().SavePendingContribution(
          contribution_iter_->publisher_id, contribution_iter_->amount,
          contribution_iter_->created_at);
    }

    contribution_iter_->completed = true;
    SaveState();
    SendNext();
  }

  std::vector<PendingContributionState>::iterator contribution_iter_;
};

}  // namespace

Future<bool> PendingContributionProcessor::Initialize() {
  context().Get<JobStore>().ResumeJobs<ProcessJob>();
  return MakeReadyFuture(true);
}

void PendingContributionProcessor::ProcessPendingContributions() {
  auto job_ids = context().Get<JobStore>().GetActiveJobs(ProcessJob::kJobType);
  if (job_ids.empty()) {
    context().Get<JobStore>().StartJobWithState<ProcessJob>({});
  }
}

}  // namespace ledger
