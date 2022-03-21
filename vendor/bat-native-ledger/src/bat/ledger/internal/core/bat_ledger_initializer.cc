/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/bat_ledger_initializer.h"

#include <utility>

#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace ledger {

namespace {

class LedgerImplInitializer : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "ledger-impl-initializer";

  Future<bool> Initialize() { return context().StartJob<Job>(); }

 private:
  class Job : public BATLedgerJob<bool> {
   public:
    void Start() {
      context().GetLedgerImpl()->database()->Initialize(
          false, ContinueWithLambda(this, &Job::OnDatabaseInitialized));
    }

   private:
    void OnDatabaseInitialized(mojom::Result result) {
      if (result != mojom::Result::LEDGER_OK) {
        context().LogError(FROM_HERE)
            << "Failed to initialize database: " << result;
        return Complete(false);
      }

      context().GetLedgerImpl()->state()->Initialize(
          ContinueWithLambda(this, &Job::OnStateInitialized));
    }

    void OnStateInitialized(mojom::Result result) {
      if (result != mojom::Result::LEDGER_OK) {
        context().LogError(FROM_HERE)
            << "Failed to initialize state: " << result;
        return Complete(false);
      }

      StartServices();
    }

    void StartServices() {
      auto* ledger = context().GetLedgerImpl();

      ledger->publisher()->SetPublisherServerListTimer();
      ledger->contribution()->SetReconcileTimer();
      ledger->promotion()->Refresh(false);
      ledger->contribution()->Initialize();
      ledger->promotion()->Initialize();
      ledger->api()->Initialize();
      ledger->recovery()->Check();

      Complete(true);
    }
  };
};

template <typename... Ts>
class InitializeJob : public BATLedgerJob<bool> {
 public:
  void Start() { StartNext<Ts..., End>(); }

 private:
  struct End {};

  template <typename T, typename... Rest>
  void StartNext() {
    context().LogVerbose(FROM_HERE) << "Initializing " << T::kContextKey;
    context().template Get<T>().Initialize().Then(
        ContinueWith(this, &InitializeJob::OnCompleted<T, Rest...>));
  }

  template <>
  void StartNext<End>() {
    context().LogVerbose(FROM_HERE) << "Initialization complete";
    Complete(true);
  }

  template <typename T, typename... Rest>
  void OnCompleted(bool success) {
    if (!success) {
      context().LogError(FROM_HERE) << "Error initializing " << T::kContextKey;
      return Complete(false);
    }
    StartNext<Rest...>();
  }
};

using InitializeAllJob = InitializeJob<LedgerImplInitializer>;

}  // namespace

BATLedgerInitializer::BATLedgerInitializer() = default;

BATLedgerInitializer::~BATLedgerInitializer() = default;

Future<bool> BATLedgerInitializer::Initialize() {
  if (!initialize_future_) {
    initialize_future_ = SharedFuture(context().StartJob<InitializeAllJob>());
  }

  return initialize_future_->Then(
      base::BindOnce([](const bool& success) { return success; }));
}

}  // namespace ledger
