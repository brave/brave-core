/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_JOB_STORE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_JOB_STORE_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/values.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/future.h"

namespace ledger {

class JobStore : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "job-store";

  JobStore();
  ~JobStore() override;

  Future<bool> Initialize();

  template <typename T>
  std::string InitializeJobState(const typename T::State& state) {
    return AddState(T::kJobType, state.ToValue());
  }

  template <typename T>
  auto StartJobWithState(const typename T::State& state) {
    return context().StartJob<T>(InitializeJobState<T>(state));
  }

  template <typename T>
  void ResumeJobs() {
    for (auto& id : GetActiveJobs(T::kJobType)) {
      context().StartJob<T>(id);
    }
  }

  std::string AddState(const std::string& job_type, const base::Value& value);

  template <typename T>
  std::string AddState(const std::string& job_type, const T& state) {
    return AddState(job_type, state.ToValue());
  }

  std::string AddCompletedState(const std::string& job_type,
                                const base::Value& value);

  template <typename T>
  std::string AddCompletedState(const std::string& job_type, const T& state) {
    return AddCompletedState(job_type, state.ToValue());
  }

  void SetState(const std::string& job_id, const base::Value& value);

  template <typename T>
  void SetState(const std::string& job_id, const T& state) {
    SetState(job_id, state.ToValue());
  }

  absl::optional<base::Value> GetState(const std::string& job_id);

  template <typename T>
  absl::optional<T> GetState(const std::string& job_id) {
    auto value = GetState(job_id);
    if (!value) {
      return {};
    }
    return T::FromValue(*value);
  }

  void OnJobCompleted(const std::string& job_id);

  void OnJobCompleted(const std::string& job_id, const std::string& error);

  std::vector<std::string> GetActiveJobs(const std::string& job_type);

 private:
  struct StateMapValue {
    std::string job_type;
    base::Value value;
  };

  std::map<std::string, StateMapValue> state_map_;
  base::WeakPtrFactory<JobStore> weak_factory_{this};
};

template <typename R, typename S>
class ResumableJob : public BATLedgerJob<R> {
 public:
  using State = S;

  void Start(const std::string& job_id) {
    job_id_ = job_id;
    state_ = GetJobStore().template GetState<S>(job_id_);
    if (state_) {
      Resume();
    } else {
      this->context().LogError(FROM_HERE)
          << "Invalid state for job " << job_id_;
      OnStateInvalid();
    }
  }

  void Complete(R result) override {
    GetJobStore().OnJobCompleted(job_id_);
    BATLedgerJob<R>::Complete(std::move(result));
  }

  void CompleteWithError(R result, const std::string& error) {
    GetJobStore().OnJobCompleted(job_id_, error);
    BATLedgerJob<R>::Complete(std::move(result));
  }

 protected:
  virtual void Resume() = 0;

  virtual void OnStateInvalid() = 0;

  std::string& job_id() { return job_id_; }
  const std::string& job_id() const { return job_id_; }

  S& state() { return *state_; }
  const S& state() const { return *state_; }

  void SaveState() { GetJobStore().SetState(job_id_, *state_); }

 private:
  JobStore& GetJobStore() { return this->context().template Get<JobStore>(); }

  std::string job_id_;
  absl::optional<S> state_;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_JOB_STORE_H_
