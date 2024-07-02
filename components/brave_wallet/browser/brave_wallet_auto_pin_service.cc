// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/brave_wallet_auto_pin_service.h"

#include <optional>
#include <vector>

#include "brave/components/brave_wallet/browser/pref_names.h"

namespace brave_wallet {
namespace {
bool ShouldRetryOnError(const mojom::PinErrorPtr& error) {
  return !error || error->error_code !=
                       mojom::WalletPinServiceErrorCode::ERR_NON_IPFS_TOKEN_URL;
}

std::optional<std::string> GetTokenStringValue(
    const mojom::BlockchainTokenPtr& token) {
  return BraveWalletPinService::GetTokenPrefPath(std::nullopt, token);
}

}  // namespace

BraveWalletAutoPinService::IntentData::IntentData(
    const BlockchainTokenPtr& token,
    Operation operation,
    std::optional<std::string> service)
    : token(token.Clone()), operation(operation), service(std::move(service)) {}

BraveWalletAutoPinService::IntentData::~IntentData() = default;

bool BraveWalletAutoPinService::IntentData::Equals(
    const std::unique_ptr<BraveWalletAutoPinService::IntentData>& other) {
  if (!other) {
    return false;
  }
  return (this->operation == other->operation &&
          BraveWalletPinService::GetTokenPrefPath(this->service, this->token) ==
              BraveWalletPinService::GetTokenPrefPath(other->service,
                                                      other->token));
}

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

void BraveWalletAutoPinService::ResetLocalState() {
  tasks_weak_ptr_factory_.InvalidateWeakPtrs();
  tokens_.clear();
  queue_.clear();
  current_.reset();
}

void BraveWalletAutoPinService::Reset() {
  ResetLocalState();
  SetAutoPinEnabled(false);
  brave_wallet_pin_service_->Reset(base::DoNothing());
}

void BraveWalletAutoPinService::OnAutoPinStatusChanged() {
  auto enabled = IsAutoPinEnabled();
  if (enabled) {
    Restore();
  } else {
    ResetLocalState();
  }
  for (const auto& observer : observers_) {
    observer->OnAutoPinStatusChanged(enabled);
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
  auto token_str = GetTokenStringValue(token);
  if (!token_str) {
    return;
  }
  tokens_.insert(token_str.value());
  std::erase_if(queue_, [&token_str](const auto& intent) {
    return GetTokenStringValue(intent->token) == token_str;
  });
  AddOrExecute(
      std::make_unique<IntentData>(token, Operation::kAdd, std::nullopt));
}

void BraveWalletAutoPinService::OnTokenRemoved(BlockchainTokenPtr token) {
  if (!IsAutoPinEnabled()) {
    return;
  }
  if (!BraveWalletPinService::IsTokenSupportedForPinning(token)) {
    return;
  }
  auto token_str = GetTokenStringValue(token);
  if (!token_str) {
    return;
  }
  tokens_.erase(token_str.value());
  std::erase_if(queue_, [&token_str](const auto& intent) {
    return GetTokenStringValue(intent->token) == token_str;
  });
  AddOrExecute(
      std::make_unique<IntentData>(token, Operation::kDelete, std::nullopt));
}

void BraveWalletAutoPinService::Restore() {
  if (!IsAutoPinEnabled()) {
    NOTREACHED_IN_MIGRATION();
    return;
  }
  tokens_.clear();
  brave_wallet_pin_service_->Restore();
  brave_wallet_service_->GetAllUserAssets(
      base::BindOnce(&BraveWalletAutoPinService::OnTokenListResolved,
                     tasks_weak_ptr_factory_.GetWeakPtr()));
}

void BraveWalletAutoPinService::OnTokenListResolved(
    std::vector<BlockchainTokenPtr> token_list) {
  if (!IsAutoPinEnabled()) {
    return;
  }
  // Resolves list of user tokens.
  // Check whether they are pinned or not and posts corresponding tasks.
  std::set<std::string> known_tokens =
      brave_wallet_pin_service_->GetTokens(std::nullopt);
  for (const auto& token : token_list) {
    if (!BraveWalletPinService::IsTokenSupportedForPinning(token)) {
      continue;
    }

    auto token_path =
        BraveWalletPinService::GetTokenPrefPath(std::nullopt, token);
    auto token_str = GetTokenStringValue(token);

    // Currently they are same but may be different in the future
    if (!token_path || !token_str) {
      continue;
    }

    known_tokens.erase(token_path.value());
    tokens_.insert(token_str.value());

    mojom::TokenPinStatusPtr status =
        brave_wallet_pin_service_->GetTokenStatus(std::nullopt, token);

    std::unique_ptr<IntentData> intent;
    if (!status) {
      AddOrExecute(
          std::make_unique<IntentData>(token, Operation::kAdd, std::nullopt));
    } else if (status->code == mojom::TokenPinStatusCode::STATUS_PINNED) {
      // Pinned tokens should be verified for entirety time to time.
      // We should check that related CIDs are still pinned.
      auto t1 = status->validate_time;
      if ((base::Time::Now() - t1) > base::Days(1) || t1 > base::Time::Now()) {
        AddOrExecute(std::make_unique<IntentData>(token, Operation::kValidate,
                                                  std::nullopt));
      }
    } else if (status->code ==
               mojom::TokenPinStatusCode::STATUS_PINNING_FAILED) {
      if (ShouldRetryOnError(status->error)) {
        AddOrExecute(
            std::make_unique<IntentData>(token, Operation::kAdd, std::nullopt));
      }
    } else {
      AddOrExecute(
          std::make_unique<IntentData>(token, Operation::kAdd, std::nullopt));
    }
  }

  // Tokens that were previously pinned but not listed in the wallet should be
  // unpinned.
  for (const auto& t : known_tokens) {
    mojom::BlockchainTokenPtr token =
        BraveWalletPinService::TokenFromPrefPath(t);
    if (token) {
      AddOrExecute(std::make_unique<IntentData>(token, Operation::kDelete,
                                                std::nullopt));
    }
  }

  CheckQueue();
}

void BraveWalletAutoPinService::ValidateToken(
    const std::unique_ptr<IntentData>& data) {
  brave_wallet_pin_service_->Validate(
      data->token->Clone(), data->service,
      base::BindOnce(&BraveWalletAutoPinService::OnValidateTaskFinished,
                     tasks_weak_ptr_factory_.GetWeakPtr()));
}

void BraveWalletAutoPinService::PinToken(
    const std::unique_ptr<IntentData>& data) {
  brave_wallet_pin_service_->AddPin(
      data->token->Clone(), data->service,
      base::BindOnce(&BraveWalletAutoPinService::OnTaskFinished,
                     tasks_weak_ptr_factory_.GetWeakPtr()));
}

void BraveWalletAutoPinService::UnpinToken(
    const std::unique_ptr<IntentData>& data) {
  brave_wallet_pin_service_->RemovePin(
      data->token->Clone(), data->service,
      base::BindOnce(&BraveWalletAutoPinService::OnTaskFinished,
                     tasks_weak_ptr_factory_.GetWeakPtr()));
}

void BraveWalletAutoPinService::AddOrExecute(std::unique_ptr<IntentData> data) {
  if (!IsAutoPinEnabled()) {
    return;
  }
  DCHECK(data);
  for (const auto& v : queue_) {
    if (v->Equals(data)) {
      return;
    }
  }
  if (current_ && current_->Equals(data)) {
    return;
  }

  auto token_str = GetTokenStringValue(data->token);
  if (!token_str) {
    return;
  }

  if (data->operation == Operation::kAdd ||
      data->operation == Operation::kValidate) {
    if (!base::Contains(tokens_, token_str.value())) {
      return;
    }
  }
  if (data->operation == Operation::kDelete) {
    if (base::Contains(tokens_, token_str.value())) {
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
                     tasks_weak_ptr_factory_.GetWeakPtr(), std::move(data)),
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

void BraveWalletAutoPinService::AddObserver(
    ::mojo::PendingRemote<mojom::WalletAutoPinServiceObserver> observer) {
  observers_.Add(std::move(observer));
}

}  // namespace brave_wallet
