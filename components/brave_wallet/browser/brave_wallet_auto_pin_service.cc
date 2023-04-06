// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/brave_wallet_auto_pin_service.h"

#include "brave/components/brave_wallet/browser/pref_names.h"

namespace brave_wallet {
namespace {
bool ShouldRetryOnError(const mojom::PinErrorPtr& error) {
  return !error || error->error_code !=
                       mojom::WalletPinServiceErrorCode::ERR_NON_IPFS_TOKEN_URL;
}
}  // namespace

BraveWalletAutoPinService::IntentData::IntentData(
    const BlockchainTokenPtr& token,
    Operation operation,
    absl::optional<std::string> service)
    : token(token.Clone()), operation(operation), service(std::move(service)) {}

BraveWalletAutoPinService::IntentData::~IntentData() = default;

BraveWalletAutoPinService::BraveWalletAutoPinService(
    PrefService* prefs,
    BraveWalletService* brave_wallet_service,
    BraveWalletPinService* brave_wallet_pin_service)
    : pref_service_(prefs),
      brave_wallet_service_(brave_wallet_service),
      brave_wallet_pin_service_(brave_wallet_pin_service) {
  DCHECK(brave_wallet_service);
  brave_wallet_service->AddTokenObserver(
      token_observer_.BindNewPipeAndPassRemote());
  if (IsAutoPinEnabled()) {
    Restore();
  }
  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(pref_service_);
  pref_change_registrar_->Add(
      kAutoPinEnabled,
      base::BindRepeating(&BraveWalletAutoPinService::OnAutoPinStatusChanged,
                          weak_ptr_factory_.GetWeakPtr()));
  brave_wallet_service->AddObserver(
      brave_wallet_service_observer_.BindNewPipeAndPassRemote());
}

void BraveWalletAutoPinService::OnResetWallet() {
  Reset();
}

void BraveWalletAutoPinService::Reset() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  queue_.clear();
  current_.reset();
  SetAutoPinEnabled(false);
  brave_wallet_pin_service_->Reset(base::DoNothing());
}

void BraveWalletAutoPinService::OnAutoPinStatusChanged() {
  if (IsAutoPinEnabled()) {
    Restore();
  } else {
    queue_.clear();
  }
}

BraveWalletAutoPinService::~BraveWalletAutoPinService() {}

void BraveWalletAutoPinService::Bind(
    mojo::PendingReceiver<mojom::WalletAutoPinService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

mojo::PendingRemote<mojom::WalletAutoPinService>
BraveWalletAutoPinService::MakeRemote() {
  mojo::PendingRemote<WalletAutoPinService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void BraveWalletAutoPinService::OnTokenAdded(BlockchainTokenPtr token) {
  if (!IsAutoPinEnabled()) {
    return;
  }
  if (!BraveWalletPinService::IsTokenSupportedForPinning(token)) {
    return;
  }
  tokens_.insert(token.Clone());
  base::EraseIf(queue_, [&token](const std::unique_ptr<IntentData>& intent) {
    return intent->token == token;
  });
  AddOrExecute(
      std::make_unique<IntentData>(token, Operation::kAdd, absl::nullopt));
}

void BraveWalletAutoPinService::OnTokenRemoved(BlockchainTokenPtr token) {
  if (!IsAutoPinEnabled()) {
    return;
  }
  if (!BraveWalletPinService::IsTokenSupportedForPinning(token)) {
    return;
  }
  tokens_.erase(token);
  base::EraseIf(queue_, [&token](const std::unique_ptr<IntentData>& intent) {
    return intent->token == token;
  });
  AddOrExecute(
      std::make_unique<IntentData>(token, Operation::kDelete, absl::nullopt));
}

void BraveWalletAutoPinService::Restore() {
  if (!IsAutoPinEnabled()) {
    NOTREACHED();
    return;
  }
  brave_wallet_pin_service_->Restore();
  brave_wallet_service_->GetAllUserAssets(
      base::BindOnce(&BraveWalletAutoPinService::OnTokenListResolved,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveWalletAutoPinService::OnTokenListResolved(
    std::vector<BlockchainTokenPtr> token_list) {
  if (!IsAutoPinEnabled()) {
    return;
  }
  // Resolves list of user tokens.
  // Check whether they are pinned or not and posts corresponding tasks.
  std::set<std::string> known_tokens =
      brave_wallet_pin_service_->GetTokens(absl::nullopt);
  for (const auto& token : token_list) {
    if (!BraveWalletPinService::IsTokenSupportedForPinning(token)) {
      continue;
    }

    tokens_.insert(token.Clone());

    auto current_token_path =
        BraveWalletPinService::GetTokenPrefPath(absl::nullopt, token);
    if (!current_token_path) {
      continue;
    }

    known_tokens.erase(current_token_path.value());

    mojom::TokenPinStatusPtr status =
        brave_wallet_pin_service_->GetTokenStatus(absl::nullopt, token);

    if (!status ||
        status->code == mojom::TokenPinStatusCode::STATUS_NOT_PINNED) {
      AddOrExecute(
          std::make_unique<IntentData>(token, Operation::kAdd, absl::nullopt));
    } else if (status->code ==
               mojom::TokenPinStatusCode::STATUS_PINNING_FAILED) {
      if (ShouldRetryOnError(status->error)) {
        AddOrExecute(std::make_unique<IntentData>(token, Operation::kAdd,
                                                  absl::nullopt));
      }
    } else if (status->code ==
                   mojom::TokenPinStatusCode::STATUS_PINNING_IN_PROGRESS ||
               status->code ==
                   mojom::TokenPinStatusCode::STATUS_PINNING_PENDING) {
      AddOrExecute(
          std::make_unique<IntentData>(token, Operation::kAdd, absl::nullopt));
    } else if (status->code ==
                   mojom::TokenPinStatusCode::STATUS_UNPINNING_FAILED ||
               status->code ==
                   mojom::TokenPinStatusCode::STATUS_UNPINNING_IN_PROGRESS ||
               status->code ==
                   mojom::TokenPinStatusCode::STATUS_UNPINNING_PENDING) {
      AddOrExecute(std::make_unique<IntentData>(token, Operation::kDelete,
                                                absl::nullopt));
    } else if (status->code == mojom::TokenPinStatusCode::STATUS_PINNED) {
      // Pinned tokens should be verified for entirety time to time.
      // We should check that related CIDs are still pinned.
      auto t1 = status->validate_time;
      if ((base::Time::Now() - t1) > base::Days(1) || t1 > base::Time::Now()) {
        AddOrExecute(std::make_unique<IntentData>(token, Operation::kValidate,
                                                  absl::nullopt));
      }
    }
  }

  // Tokens that were previously pinned but not listed in the wallet should be
  // unpinned.
  for (const auto& t : known_tokens) {
    mojom::BlockchainTokenPtr token =
        BraveWalletPinService::TokenFromPrefPath(t);
    if (token) {
      AddOrExecute(std::make_unique<IntentData>(token, Operation::kDelete,
                                                absl::nullopt));
    }
  }

  CheckQueue();
}

void BraveWalletAutoPinService::ValidateToken(
    const std::unique_ptr<IntentData>& data) {
  brave_wallet_pin_service_->Validate(
      data->token->Clone(), data->service,
      base::BindOnce(&BraveWalletAutoPinService::OnValidateTaskFinished,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveWalletAutoPinService::PinToken(
    const std::unique_ptr<IntentData>& data) {
  brave_wallet_pin_service_->AddPin(
      data->token->Clone(), data->service,
      base::BindOnce(&BraveWalletAutoPinService::OnTaskFinished,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveWalletAutoPinService::UnpinToken(
    const std::unique_ptr<IntentData>& data) {
  brave_wallet_pin_service_->RemovePin(
      data->token->Clone(), data->service,
      base::BindOnce(&BraveWalletAutoPinService::OnTaskFinished,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveWalletAutoPinService::AddOrExecute(std::unique_ptr<IntentData> data) {
  if (!IsAutoPinEnabled()) {
    return;
  }
  DCHECK(data);
  for (const auto& v : queue_) {
    if (v->token == data->token && v->service == data->service &&
        v->operation == data->operation) {
      return;
    }
  }
  if (current_ && current_->token == data->token &&
      current_->service == data->service &&
      current_->operation == data->operation) {
    return;
  }

  if (data->operation == Operation::kAdd ||
      data->operation == Operation::kValidate) {
    if (tokens_.find(data->token) == tokens_.end()) {
      return;
    }
  }
  if (data->operation == Operation::kDelete) {
    if (tokens_.find(data->token) != tokens_.end()) {
      return;
    }
  }

  if (data->operation == Operation::kAdd) {
    brave_wallet_pin_service_->MarkAsPendingForPinning(data->token,
                                                       data->service);
  } else if (data->operation == Operation::kDelete) {
    brave_wallet_pin_service_->MarkAsPendingForUnpinning(data->token,
                                                         data->service);
  }
  queue_.push_back(std::move(data));
  CheckQueue();
}

void BraveWalletAutoPinService::PostRetry(std::unique_ptr<IntentData> data) {
  if (!IsAutoPinEnabled()) {
    return;
  }
  int multiply = ++data->attempt;
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&BraveWalletAutoPinService::AddOrExecute,
                     weak_ptr_factory_.GetWeakPtr(), std::move(data)),
      base::Minutes(2 * multiply));
}

void BraveWalletAutoPinService::CheckQueue() {
  if (!IsAutoPinEnabled()) {
    return;
  }
  if (queue_.empty() || current_) {
    return;
  }

  current_ = std::move(queue_.front());
  queue_.pop_front();

  if (current_->operation == Operation::kAdd) {
    PinToken(current_);
  } else if (current_->operation == Operation::kDelete) {
    UnpinToken(current_);
  } else if (current_->operation == Operation::kValidate) {
    ValidateToken(current_);
  }
}

void BraveWalletAutoPinService::OnTaskFinished(bool result,
                                               mojom::PinErrorPtr error) {
  CHECK(current_);
  if (!result &&
      (current_->operation != Operation::kAdd || ShouldRetryOnError(error))) {
    PostRetry(std::move(current_));
  }
  current_.reset();
  CheckQueue();
}

void BraveWalletAutoPinService::OnValidateTaskFinished(
    mojom::TokenValidationResult result) {
  if (result == mojom::TokenValidationResult::kValidationError) {
    PostRetry(std::move(current_));
  }
  auto current = std::move(current_);
  if (result == mojom::TokenValidationResult::kValidationFailed) {
    AddOrExecute(std::make_unique<IntentData>(current->token, Operation::kAdd,
                                              current->service));
  }
  CheckQueue();
}

void BraveWalletAutoPinService::SetAutoPinEnabled(bool enabled) {
  pref_service_->SetBoolean(kAutoPinEnabled, enabled);
}

bool BraveWalletAutoPinService::IsAutoPinEnabled() {
  return pref_service_->GetBoolean(kAutoPinEnabled);
}

void BraveWalletAutoPinService::IsAutoPinEnabled(
    IsAutoPinEnabledCallback callback) {
  std::move(callback).Run(IsAutoPinEnabled());
}

}  // namespace brave_wallet
