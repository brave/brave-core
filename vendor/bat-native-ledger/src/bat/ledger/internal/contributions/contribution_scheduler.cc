/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contributions/contribution_scheduler.h"

#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/timer/timer.h"
#include "bat/ledger/internal/contributions/auto_contribute_processor.h"
#include "bat/ledger/internal/contributions/contribution_data.h"
#include "bat/ledger/internal/contributions/contribution_router.h"
#include "bat/ledger/internal/contributions/contribution_store.h"
#include "bat/ledger/internal/contributions/recurring_contribution_processor.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/future_join.h"
#include "bat/ledger/internal/core/job_store.h"
#include "bat/ledger/internal/core/user_prefs.h"
#include "bat/ledger/internal/core/value_converters.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/publisher/publisher_service.h"

namespace ledger {

namespace {

enum class ScheduledContributionStatus { kPending, kRecurringSent, kComplete };

std::string StringifyEnum(ScheduledContributionStatus status) {
  switch (status) {
    case ScheduledContributionStatus::kPending:
      return "pending";
    case ScheduledContributionStatus::kRecurringSent:
      return "recurring-sent";
    case ScheduledContributionStatus::kComplete:
      return "complete";
  }
}

absl::optional<ScheduledContributionStatus> ParseEnum(
    const EnumString<ScheduledContributionStatus>& s) {
  return s.Match({ScheduledContributionStatus::kPending,
                  ScheduledContributionStatus::kRecurringSent,
                  ScheduledContributionStatus::kComplete});
}

struct ScheduledContributionState {
  std::vector<RecurringContribution> recurring;
  std::vector<PublisherActivity> activity;
  std::string recurring_contribution_job_id;
  std::string auto_contribute_job_id;
  ScheduledContributionStatus status = ScheduledContributionStatus::kPending;

  auto ToValue() const {
    ValueWriter w;
    w.Write("recurring", recurring);
    w.Write("activity", activity);
    w.Write("recurring_contribution_job_id", recurring_contribution_job_id);
    w.Write("auto_contribute_job_id", auto_contribute_job_id);
    w.Write("status", status);
    return w.Finish();
  }

  static auto FromValue(const base::Value& value) {
    StructValueReader<ScheduledContributionState> r(value);
    r.Read("recurring", &ScheduledContributionState::recurring);
    r.Read("activity", &ScheduledContributionState::activity);
    r.Read("recurring_contribution_job_id",
           &ScheduledContributionState::recurring_contribution_job_id);
    r.Read("auto_contribute_job_id",
           &ScheduledContributionState::auto_contribute_job_id);
    r.Read("status", &ScheduledContributionState::status);
    return r.Finish();
  }
};

class ContributionJob : public ResumableJob<bool, ScheduledContributionState> {
 public:
  static constexpr char kJobType[] = "scheduled-contribution";

 protected:
  void Resume() override {
    switch (state().status) {
      case ScheduledContributionStatus::kPending:
        return SendRecurring();
      case ScheduledContributionStatus::kRecurringSent:
        return SendAutoContribute();
      case ScheduledContributionStatus::kComplete:
        return Complete(true);
    }
  }

  void OnStateInvalid() override { Complete(false); }

 private:
  void SendRecurring() {
    if (state().recurring_contribution_job_id.empty()) {
      if (state().recurring.empty()) {
        context().LogVerbose(FROM_HERE) << "No recurring contributions to send";
        return OnRecurringSent(true);
      }
      StartRecurring();
    }

    context()
        .Get<RecurringContributionProcessor>()
        .ResumeContributions(state().recurring_contribution_job_id)
        .Then(ContinueWith(this, &ContributionJob::OnRecurringSent));
  }

  void StartRecurring() {
    state().recurring_contribution_job_id =
        context().Get<RecurringContributionProcessor>().StartContributions(
            state().recurring);

    SaveState();
  }

  void OnRecurringSent(bool success) {
    if (!success) {
      context().LogError(FROM_HERE) << "Unable to send recurring contributions";
    }

    state().status = ScheduledContributionStatus::kRecurringSent;
    SaveState();
    SendAutoContribute();
  }

  void SendAutoContribute() {
    if (state().auto_contribute_job_id.empty()) {
      // Before starting the auto-contribute processor, perform a final check
      // to determine whether the user has disabled AC. If they have disabled
      // AC, then clear activity information for this job.
      if (!context().Get<UserPrefs>().ac_enabled()) {
        context().LogVerbose(FROM_HERE) << "Auto contribute has been disabled";
        state().activity.clear();
        SaveState();
      }

      if (state().activity.empty()) {
        context().LogVerbose(FROM_HERE) << "No auto contributions to send";
        return OnAutoContributeComplete(true);
      }

      StartAutoContribute();
    }

    context()
        .Get<AutoContributeProcessor>()
        .ResumeContributions(state().auto_contribute_job_id)
        .Then(ContinueWith(this, &ContributionJob::OnAutoContributeComplete));
  }

  void StartAutoContribute() {
    auto& prefs = context().Get<UserPrefs>();
    auto source = context().Get<ContributionRouter>().GetCurrentSource();

    state().auto_contribute_job_id =
        context().Get<AutoContributeProcessor>().StartContributions(
            source, state().activity, prefs.ac_minimum_visits(),
            prefs.ac_minimum_duration(), GetAutoContributeAmount());

    SaveState();
  }

  void OnAutoContributeComplete(bool success) {
    if (!success) {
      context().LogError(FROM_HERE) << "Unable to send auto contribution";
    }

    state().status = ScheduledContributionStatus::kComplete;
    SaveState();
    Complete(true);
  }

  double GetAutoContributeAmount() {
    double ac_amount = context().Get<UserPrefs>().ac_amount();
    if (ac_amount > 0) {
      return ac_amount;
    }
    return context().GetLedgerImpl()->state()->GetAutoContributeChoice();
  }
};

// Returns a list of publisher activity records for user activity since the
// last scheduled contribution. Publishers that are not yet registered are
// filtered out of the resulting list.
class GetActivityJob : public BATLedgerJob<std::vector<PublisherActivity>> {
 public:
  void Start() {
    if (!context().options().auto_contribute_allowed) {
      context().LogVerbose(FROM_HERE)
          << "Auto contribute is not allowed for this client";
      return Complete({});
    }

    if (!context().Get<UserPrefs>().ac_enabled()) {
      context().LogVerbose(FROM_HERE) << "Auto contribute is not enabled";
      return Complete({});
    }

    context().Get<ContributionStore>().GetPublisherActivity().Then(
        ContinueWith(this, &GetActivityJob::OnStoreRead));
  }

 private:
  void OnStoreRead(std::vector<PublisherActivity> activity) {
    activity_ = std::move(activity);

    std::vector<std::string> publisher_ids;
    for (auto& entry : activity_) {
      publisher_ids.push_back(entry.publisher_id);
    }

    context()
        .Get<PublisherService>()
        .GetPublishers(publisher_ids)
        .Then(ContinueWith(this, &GetActivityJob::OnPublishersLoaded));
  }

  void OnPublishersLoaded(std::map<std::string, Publisher> publishers) {
    std::vector<PublisherActivity> filtered_activity;
    for (auto& entry : activity_) {
      auto iter = publishers.find(entry.publisher_id);
      if (iter != publishers.end() && iter->second.registered) {
        filtered_activity.push_back(entry);
      }
    }

    Complete(std::move(filtered_activity));
  }

  std::vector<PublisherActivity> activity_;
};

class ResumeAllJob : public BATLedgerJob<bool> {
 public:
  void Start() {
    job_ids_ =
        context().Get<JobStore>().GetActiveJobs(ContributionJob::kJobType);
    id_iterator_ = job_ids_.begin();
    ResumeNextJob();
  }

 private:
  void ResumeNextJob() {
    if (id_iterator_ == job_ids_.end()) {
      return Complete(true);
    }

    std::string id = std::move(*id_iterator_);
    ++id_iterator_;

    context().StartJob<ContributionJob>(id).Then(
        ContinueWith(this, &ResumeAllJob::OnJobCompleted));
  }

  void OnJobCompleted(bool success) { ResumeNextJob(); }

  std::vector<std::string> job_ids_;
  std::vector<std::string>::iterator id_iterator_;
};

class ContributionTimer : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "contribution-timer";

  Future<bool> Start(base::Location location, base::TimeDelta delay) {
    if (fire_immediately_) {
      DCHECK(!timer_.IsRunning());
      fire_immediately_ = false;
      timer_.Stop();
      return MakeReadyFuture(true);
    }

    context().LogVerbose(location)
        << "Setting contribution timer for " << delay;

    Promise<bool> promise;
    Future<bool> future = promise.GetFuture();

    timer_.Start(
        location, delay,
        base::BindOnce([](Promise<bool> promise) { promise.SetValue(true); },
                       std::move(promise)));

    return future;
  }

  void FireNow() {
    if (timer_.IsRunning()) {
      timer_.FireNow();
    } else {
      fire_immediately_ = true;
    }
  }

 private:
  bool fire_immediately_ = false;
  base::OneShotTimer timer_;
};

class SchedulerJob : public BATLedgerJob<bool> {
 public:
  void Start() {
    context().StartJob<ResumeAllJob>().Then(
        ContinueWith(this, &SchedulerJob::OnResumedJobsCompleted));
  }

 private:
  void OnResumedJobsCompleted(bool) { ScheduleNext(); }

  void ScheduleNext() {
    context()
        .Get<ContributionScheduler>()
        .GetNextScheduledContributionTime()
        .Then(ContinueWith(this, &SchedulerJob::OnNextTimeRead));
  }

  void OnNextTimeRead(base::Time time) {
    context()
        .Get<ContributionTimer>()
        .Start(FROM_HERE, time - base::Time::Now())
        .Then(ContinueWith(this, &SchedulerJob::OnTimerElapsed));
  }

  void OnTimerElapsed(bool) {
    context().LogVerbose(FROM_HERE) << "Starting scheduled contributions";

    auto contributions_future =
        context().Get<ContributionStore>().GetRecurringContributions();

    auto activity_future = context().StartJob<GetActivityJob>();

    JoinFutures(contributions_future, activity_future)
        .Then(ContinueWith(this, &SchedulerJob::OnDataReady));
  }

  void OnDataReady(std::tuple<std::vector<RecurringContribution>,
                              std::vector<PublisherActivity>> data) {
    auto& [recurring, activity] = data;

    auto& store = context().Get<ContributionStore>();
    store.UpdateLastScheduledContributionTime();
    store.ResetPublisherActivity();

    context()
        .Get<JobStore>()
        .StartJobWithState<ContributionJob>({.recurring = std::move(recurring),
                                             .activity = std::move(activity)})
        .Then(ContinueWith(this, &SchedulerJob::OnContributionsSent));
  }

  void OnContributionsSent(bool) { ScheduleNext(); }
};

}  // namespace

Future<bool> ContributionScheduler::Initialize() {
  if (context().options().enable_experimental_features) {
    context().StartJob<SchedulerJob>();
  }
  return MakeReadyFuture(true);
}

void ContributionScheduler::StartContributions() {
  context().Get<ContributionTimer>().FireNow();
}

Future<base::Time> ContributionScheduler::GetNextScheduledContributionTime() {
  return context()
      .Get<ContributionStore>()
      .GetLastScheduledContributionTime()
      .Then(base::BindOnce([](base::Time last) {
        return last + kScheduledContributionInterval;
      }));
}

}  // namespace ledger
