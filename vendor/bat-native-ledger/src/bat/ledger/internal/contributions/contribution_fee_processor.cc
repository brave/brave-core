/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contributions/contribution_fee_processor.h"

#include <string>
#include <utility>
#include <vector>

#include "base/time/time.h"
#include "base/values.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/delay_generator.h"
#include "bat/ledger/internal/core/job_store.h"
#include "bat/ledger/internal/core/value_converters.h"
#include "bat/ledger/internal/external_wallet/external_wallet_manager.h"

namespace ledger {

namespace {

constexpr base::TimeDelta kFeeDelay = base::Seconds(45);

constexpr int kMaxRetries = 3;

static const char kFeeDescription[] =
    "5% transaction fee collected by Brave Software International";

struct FeeState {
  std::string contribution_id;
  double amount = 0;
  absl::optional<ExternalWalletProvider> provider;
  std::string transaction_id;

  auto ToValue() const {
    ValueWriter w;
    w.Write("contribution_id", contribution_id);
    w.Write("amount", amount);
    w.Write("provider", provider);
    w.Write("transaction_id", transaction_id);
    return w.Finish();
  }

  static auto FromValue(const base::Value& value) {
    StructValueReader<FeeState> r(value);
    r.Read("contribution_id", &FeeState::contribution_id);
    r.Read("amount", &FeeState::amount);
    r.Read("provider", &FeeState::provider);
    r.Read("transaction_id", &FeeState::transaction_id);
    return r.Finish();
  }
};

class FeeJob : public ResumableJob<bool, FeeState> {
 public:
  static constexpr char kJobType[] = "contribution-fee";

 protected:
  void Resume() override { ProcessAfterDelay(); }

  void OnStateInvalid() override { Complete(false); }

 private:
  void ProcessAfterDelay() {
    context()
        .Get<DelayGenerator>()
        .RandomDelay(FROM_HERE, kFeeDelay)
        .Then(ContinueWith(this, &FeeJob::OnDelay));
  }

  void OnDelay(base::TimeDelta) {
    context().LogVerbose(FROM_HERE)
        << "Sending fee for contribution " << state().contribution_id;

    context()
        .Get<ExternalWalletManager>()
        .TransferBAT(GetFeeAddress(), state().amount, kFeeDescription)
        .Then(ContinueWith(this, &FeeJob::OnTransferCompleted));
  }

  void OnTransferCompleted(
      absl::optional<ExternalWalletTransferResult> result) {
    if (!result) {
      if (retry_count_ >= kMaxRetries) {
        context().LogError(FROM_HERE) << "Unable to send fee for contribution "
                                      << state().contribution_id;
        return CompleteWithError(false, "transfer-failed");
      }

      context().LogError(FROM_HERE)
          << "Error sending fee for contribution " << state().contribution_id;

      retry_count_ += 1;
      return ProcessAfterDelay();
    }

    state().provider = result->provider;
    state().transaction_id = result->transaction_id;
    SaveState();

    return Complete(true);
  }

  std::string GetFeeAddress() {
    auto address =
        context().Get<ExternalWalletManager>().GetContributionFeeAddress();
    return address ? *address : "";
  }

  int retry_count_ = 0;
};

}  // namespace

Future<bool> ContributionFeeProcessor::Initialize() {
  context().Get<JobStore>().ResumeJobs<FeeJob>();
  return MakeReadyFuture(true);
}

void ContributionFeeProcessor::SendContributionFee(
    const std::string& contribution_id,
    double fee_amount) {
  DCHECK(!contribution_id.empty());
  DCHECK_GT(fee_amount, 0.0);

  context().Get<JobStore>().StartJobWithState<FeeJob>(
      {.contribution_id = contribution_id, .amount = fee_amount});
}

}  // namespace ledger
