/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contributions/contribution_token_vendor.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/numerics/clamped_math.h"
#include "bat/ledger/internal/contributions/contribution_token_manager.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/delay_generator.h"
#include "bat/ledger/internal/core/environment_config.h"
#include "bat/ledger/internal/core/job_store.h"
#include "bat/ledger/internal/core/privacy_pass.h"
#include "bat/ledger/internal/core/value_converters.h"
#include "bat/ledger/internal/external_wallet/external_wallet_manager.h"
#include "bat/ledger/internal/payments/payment_service.h"

namespace ledger {

namespace {

constexpr double kVotePrice = 0.25;
constexpr base::TimeDelta kMinRetryDelay = base::Seconds(15);
constexpr base::TimeDelta kMaxRetryDelay = base::Minutes(30);

enum class PurchaseStatus {
  kPending,
  kOrderCreated,
  kTransferCompleted,
  kTransactionSent,
  kOrderPaid,
  kTokensCreated,
  kTokensClaimed,
  kComplete
};

std::string StringifyEnum(PurchaseStatus status) {
  switch (status) {
    case PurchaseStatus::kPending:
      return "pending";
    case PurchaseStatus::kOrderCreated:
      return "order-created";
    case PurchaseStatus::kTransferCompleted:
      return "transfer-completed";
    case PurchaseStatus::kTransactionSent:
      return "transaction-sent";
    case PurchaseStatus::kOrderPaid:
      return "order-paid";
    case PurchaseStatus::kTokensCreated:
      return "tokens-created";
    case PurchaseStatus::kTokensClaimed:
      return "tokens-claimed";
    case PurchaseStatus::kComplete:
      return "complete";
  }
}

absl::optional<PurchaseStatus> ParseEnum(const EnumString<PurchaseStatus>& s) {
  return s.Match({PurchaseStatus::kPending, PurchaseStatus::kOrderCreated,
                  PurchaseStatus::kTransferCompleted,
                  PurchaseStatus::kTransactionSent, PurchaseStatus::kOrderPaid,
                  PurchaseStatus::kTokensCreated,
                  PurchaseStatus::kTokensClaimed, PurchaseStatus::kComplete});
}

struct PurchaseState {
  int quantity = 0;
  std::string order_id;
  std::string order_item_id;
  absl::optional<ExternalWalletProvider> external_provider;
  std::string external_transaction_id;
  std::vector<std::string> tokens;
  std::vector<std::string> blinded_tokens;
  PurchaseStatus status = PurchaseStatus::kPending;

  base::Value ToValue() const {
    ValueWriter w;
    w.Write("quantity", quantity);
    w.Write("order_id", order_id);
    w.Write("order_item_id", order_item_id);
    w.Write("external_provider", external_provider);
    w.Write("external_transaction_id", external_transaction_id);
    w.Write("tokens", tokens);
    w.Write("blinded_tokens", blinded_tokens);
    w.Write("status", status);
    return w.Finish();
  }

  static absl::optional<PurchaseState> FromValue(const base::Value& value) {
    StructValueReader<PurchaseState> r(value);
    r.Read("quantity", &PurchaseState::quantity);
    r.Read("order_id", &PurchaseState::order_id);
    r.Read("order_item_id", &PurchaseState::order_item_id);
    r.Read("external_provider", &PurchaseState::external_provider);
    r.Read("external_transaction_id", &PurchaseState::external_transaction_id);
    r.Read("tokens", &PurchaseState::tokens);
    r.Read("blinded_tokens", &PurchaseState::blinded_tokens);
    r.Read("status", &PurchaseState::status);
    return r.Finish();
  }
};

// Returns a callback for the specified argument type that discards its argument
// and calls a wrapped callback. This transform can be used to discard the value
// produced by a `Future`.
template <typename T, typename R>
base::OnceCallback<R(T)> DiscardValue(base::OnceCallback<R()> callback) {
  return base::BindOnce(
      [](decltype(callback) cb, T) { return std::move(cb).Run(); },
      std::move(callback));
}

class PurchaseJob : public ResumableJob<bool, PurchaseState> {
 public:
  static constexpr char kJobType[] = "contribution-token-purchase";

 protected:
  void Resume() override {
    switch (state().status) {
      case PurchaseStatus::kPending:
        return CreateOrder();
      case PurchaseStatus::kOrderCreated:
        return TransferFunds();
      case PurchaseStatus::kTransferCompleted:
        return SendTransaction();
      case PurchaseStatus::kTransactionSent:
        return CreateTokens();
      case PurchaseStatus::kOrderPaid:
        return WaitForTransactionCompletion();
      case PurchaseStatus::kTokensCreated:
        return ClaimTokens();
      case PurchaseStatus::kTokensClaimed:
        return FetchSignedTokens();
      case PurchaseStatus::kComplete:
        return Complete(true);
    }
  }

  void OnStateInvalid() override { Complete(false); }

 private:
  void CreateOrder() {
    if (state().quantity <= 0) {
      NOTREACHED();
      context().LogError(FROM_HERE) << "Invalid token order quantity";
      return CompleteWithError(false, "invalid-quantity");
    }

    const char* sku = context().Get<EnvironmentConfig>().auto_contribute_sku();
    std::map<std::string, int> items;
    items[sku] = state().quantity;

    context().Get<PaymentService>().CreateOrder(items).Then(
        ContinueWith(this, &PurchaseJob::OnOrderCreated));
  }

  void OnOrderCreated(absl::optional<PaymentOrder> order) {
    if (!order) {
      context().LogError(FROM_HERE) << "Error attempting to create order";
      return CompleteWithError(false, "create-order-error");
    }

    if (order->items.size() != 1) {
      context().LogError(FROM_HERE) << "Unexpected number of order items";
      return CompleteWithError(false, "invalid-item-count");
    }

    auto& item = order->items.front();
    if (item.price != kVotePrice) {
      context().LogError(FROM_HERE) << "Unexpected vote price for order item";
      return CompleteWithError(false, "invalid-vote-price");
    }

    state().order_id = order->id;
    state().order_item_id = item.id;
    state().status = PurchaseStatus::kOrderCreated;
    SaveState();

    TransferFunds();
  }

  void TransferFunds() {
    auto& manager = context().Get<ExternalWalletManager>();
    auto destination = manager.GetContributionTokenOrderAddress();
    if (!destination) {
      context().LogError(FROM_HERE)
          << "External provider does not support contribution token orders";
      return CompleteWithError(false, "invalid-provider");
    }

    double transfer_amount = state().quantity * kVotePrice;
    manager.TransferBAT(*destination, transfer_amount)
        .Then(ContinueWith(this, &PurchaseJob::OnTransferCompleted));
  }

  void OnTransferCompleted(
      absl::optional<ExternalWalletTransferResult> result) {
    if (!result) {
      context().LogError(FROM_HERE) << "External transfer failed";
      return CompleteWithError(false, "transfer-failed");
    }

    state().external_provider = result->provider;
    state().external_transaction_id = result->transaction_id;
    state().status = PurchaseStatus::kTransferCompleted;
    SaveState();

    SendTransaction();
  }

  void SendTransaction() {
    if (!state().external_provider) {
      context().LogError(FROM_HERE)
          << "AC state missing external wallet provider";
      return CompleteWithError(false, "missing-provider");
    }

    context()
        .Get<PaymentService>()
        .PostExternalTransaction(state().order_id,
                                 state().external_transaction_id,
                                 *state().external_provider)
        .Then(ContinueWith(this, &PurchaseJob::OnTransactionSent));
  }

  void OnTransactionSent(bool success) {
    if (!success) {
      context().LogError(FROM_HERE) << "Unable to send external transaction ID";
      WaitForRetryThen(ContinueWith(this, &PurchaseJob::SendTransaction));
      return;
    }

    backoff_.Reset();

    state().status = PurchaseStatus::kTransactionSent;
    SaveState();

    WaitForTransactionCompletion();
  }

  void WaitForTransactionCompletion() {
    context()
        .Get<PaymentService>()
        .GetOrder(state().order_id)
        .Then(ContinueWith(this, &PurchaseJob::OnOrderFetched));
  }

  void OnOrderFetched(absl::optional<PaymentOrder> order) {
    if (!order || order->status != PaymentOrderStatus::kPaid) {
      context().LogError(FROM_HERE) << "Order status is not 'paid' yet";
      WaitForRetryThen(
          ContinueWith(this, &PurchaseJob::WaitForTransactionCompletion));
      return;
    }

    backoff_.Reset();
    state().status = PurchaseStatus::kOrderPaid;
    SaveState();

    CreateTokens();
  }

  void CreateTokens() {
    auto batch =
        context().Get<PrivacyPass>().CreateBlindedTokens(state().quantity);

    state().tokens = std::move(batch.tokens);
    state().blinded_tokens = std::move(batch.blinded_tokens);
    state().status = PurchaseStatus::kTokensCreated;
    SaveState();

    ClaimTokens();
  }

  void ClaimTokens() {
    base::Value blinded_token_list(base::Value::Type::LIST);
    for (auto& blinded_token : state().blinded_tokens) {
      blinded_token_list.Append(blinded_token);
    }

    context()
        .Get<PaymentService>()
        .PostCredentials(state().order_id, state().order_item_id,
                         PaymentCredentialType::kSingleUse,
                         state().blinded_tokens)
        .Then(ContinueWith(this, &PurchaseJob::OnTokensClaimed));
  }

  void OnTokensClaimed(bool success) {
    if (!success) {
      context().LogError(FROM_HERE) << "Unable to claim signed tokens";
      WaitForRetryThen(ContinueWith(this, &PurchaseJob::ClaimTokens));
      return;
    }

    backoff_.Reset();

    state().status = PurchaseStatus::kTokensClaimed;
    SaveState();

    FetchSignedTokens();
  }

  void FetchSignedTokens() {
    context()
        .Get<PaymentService>()
        .GetCredentials(state().order_id, state().order_item_id)
        .Then(ContinueWith(this, &PurchaseJob::OnSignedTokensFetched));
  }

  void OnSignedTokensFetched(absl::optional<PaymentCredentials> credentials) {
    if (!credentials) {
      context().LogError(FROM_HERE) << "Unable to fetch signed tokens";
      WaitForRetryThen(ContinueWith(this, &PurchaseJob::FetchSignedTokens));
      return;
    }

    backoff_.Reset();

    auto unblinded_tokens = context().Get<PrivacyPass>().UnblindTokens(
        state().tokens, state().blinded_tokens, credentials->signed_tokens,
        credentials->batch_proof, credentials->public_key);

    if (!unblinded_tokens) {
      context().LogError(FROM_HERE) << "Error unblinding tokens";
      WaitForRetryThen(ContinueWith(this, &PurchaseJob::FetchSignedTokens));
      return;
    }

    std::vector<ContributionToken> contribution_tokens;
    for (auto& unblinded : *unblinded_tokens) {
      contribution_tokens.push_back({.value = kVotePrice,
                                     .unblinded_token = std::move(unblinded),
                                     .public_key = credentials->public_key});
    }

    context()
        .Get<ContributionTokenManager>()
        .InsertTokens(contribution_tokens, ContributionTokenType::kSKU)
        .Then(ContinueWith(this, &PurchaseJob::OnTokensInserted));
  }

  void OnTokensInserted(bool success) {
    if (!success) {
      context().LogError(FROM_HERE) << "Error saving contribution tokens";
      WaitForRetryThen(ContinueWith(this, &PurchaseJob::FetchSignedTokens));
      return;
    }

    state().status = PurchaseStatus::kComplete;
    SaveState();

    Complete(true);
  }

  void WaitForRetryThen(base::OnceCallback<void()> callback) {
    context()
        .Get<DelayGenerator>()
        .Delay(FROM_HERE, backoff_.GetNextDelay())
        .Then(DiscardValue<base::TimeDelta>(std::move(callback)));
  }

  BackoffDelay backoff_{kMinRetryDelay, kMaxRetryDelay};
};

}  // namespace

std::string ContributionTokenVendor::StartPurchase(double amount) {
  return context().Get<JobStore>().InitializeJobState<PurchaseJob>(
      {.quantity = std::max(0, base::ClampFloor(amount / kVotePrice))});
}

Future<bool> ContributionTokenVendor::ResumePurchase(
    const std::string& job_id) {
  return context().StartJob<PurchaseJob>(job_id);
}

}  // namespace ledger
