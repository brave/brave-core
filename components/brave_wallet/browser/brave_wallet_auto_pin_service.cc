// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/brave_wallet_auto_pin_service.h"

#include "brave/components/brave_wallet/browser/pref_names.h"

namespace brave_wallet {

IntentData::IntentData(const BlockchainTokenPtr& token,
                       Operation operation,
                       absl::optional<std::string> service)
    : token(token.Clone()), operation(operation), service(std::move(service)) {}

IntentData::~IntentData() {}

BraveWalletAutoPinService::BraveWalletAutoPinService(
    PrefService* prefs,
    BraveWalletService* brave_wallet_service,
    BraveWalletPinService* brave_wallet_pin_service)
    : pref_service_(prefs),
      brave_wallet_service_(brave_wallet_service),
      brave_wallet_pin_service_(brave_wallet_pin_service) {
  Restore();
  brave_wallet_service->AddTokenObserver(
      token_observer_.BindNewPipeAndPassRemote());
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
  if (!token->is_nft) {
    return;
  }
  if (!IsAutoPinEnabled()) {
    return;
  }
  PostPinToken(std::move(token), base::OnceCallback<void(bool)>());
}

void BraveWalletAutoPinService::OnTokenRemoved(BlockchainTokenPtr token) {
  if (!token->is_nft) {
    return;
  }
  base::EraseIf(queue_, [&token](const std::unique_ptr<IntentData>& intent) {
    return intent->token == token;
  });
  PostUnpinToken(std::move(token), base::OnceCallback<void(bool)>());
}

void BraveWalletAutoPinService::Restore() {
  brave_wallet_service_->GetAllUserAssets(base::BindOnce(
      &BraveWalletAutoPinService::OnTokenListResolved, base::Unretained(this)));
}

void BraveWalletAutoPinService::OnTokenListResolved(
    std::vector<BlockchainTokenPtr> token_list) {
  bool autopin_enabled = IsAutoPinEnabled();
  std::set<std::string> known_tokens =
      brave_wallet_pin_service_->GetTokens(absl::nullopt);
  for (const auto& token : token_list) {
    if (!token->is_nft) {
      continue;
    }
    std::string current_token_path =
        BraveWalletPinService::GetPath(absl::nullopt, token);

    auto it = known_tokens.find(current_token_path);
    if (it != known_tokens.end()) {
      known_tokens.erase(it);
    }

    mojom::TokenPinStatusPtr status =
        brave_wallet_pin_service_->GetTokenStatus(absl::nullopt, token);

    if (!status ||
        status->code == mojom::TokenPinStatusCode::STATUS_NOT_PINNED) {
      if (autopin_enabled) {
        AddOrExecute(std::make_unique<IntentData>(token, Operation::kAdd,
                                                  absl::nullopt));
      }
    } else if (status->code ==
                   mojom::TokenPinStatusCode::STATUS_PINNING_FAILED ||
               status->code ==
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
      auto t1 = status->validate_time;
      if ((base::Time::Now() - t1) > base::Days(1) || t1 > base::Time::Now()) {
        AddOrExecute(std::make_unique<IntentData>(token, Operation::kValidate,
                                                  absl::nullopt));
      }
    }
  }

  for (const auto& t : known_tokens) {
    mojom::BlockchainTokenPtr token = BraveWalletPinService::TokenFromPath(t);
    if (token) {
      AddOrExecute(std::make_unique<IntentData>(token, Operation::kDelete,
                                                absl::nullopt));
    }
  }

  CheckQueue();
}

void BraveWalletAutoPinService::PostPinToken(BlockchainTokenPtr token,
                                             PostPinTokenCallback callback) {
  queue_.push_back(
      std::make_unique<IntentData>(token, Operation::kAdd, absl::nullopt));
  CheckQueue();
}

void BraveWalletAutoPinService::PostUnpinToken(BlockchainTokenPtr token,
                                               PostPinTokenCallback callback) {
  queue_.push_back(
      std::make_unique<IntentData>(token, Operation::kDelete, absl::nullopt));
  CheckQueue();
}

void BraveWalletAutoPinService::ValidateToken(
    const std::unique_ptr<IntentData>& data) {
  brave_wallet_pin_service_->Validate(
      data->token->Clone(), data->service,
      base::BindOnce(&BraveWalletAutoPinService::OnValidateTaskFinished,
                     base::Unretained(this)));
}

void BraveWalletAutoPinService::PinToken(
    const std::unique_ptr<IntentData>& data) {
  brave_wallet_pin_service_->AddPin(
      data->token->Clone(), data->service,
      base::BindOnce(&BraveWalletAutoPinService::OnTaskFinished,
                     base::Unretained(this)));
}

void BraveWalletAutoPinService::UnpinToken(
    const std::unique_ptr<IntentData>& data) {
  brave_wallet_pin_service_->RemovePin(
      data->token->Clone(), data->service,
      base::BindOnce(&BraveWalletAutoPinService::OnTaskFinished,
                     base::Unretained(this)));
}

void BraveWalletAutoPinService::AddOrExecute(std::unique_ptr<IntentData> data) {
  DCHECK(data);
  for (const auto& v : queue_) {
    if (v->token == data->token && v->service == data->service) {
      return;
    }
  }
  if (current_ && current_->token == data->token &&
      current_->service == data->service) {
    return;
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
  int multiply = ++data->attempt;
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&BraveWalletAutoPinService::AddOrExecute,
                     weak_ptr_factory_.GetWeakPtr(), std::move(data)),
      base::Minutes(1 * multiply));
}

void BraveWalletAutoPinService::CheckQueue() {
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
  if (!result) {
    PostRetry(std::move(current_));
  }
  current_.reset();
  CheckQueue();
}

void BraveWalletAutoPinService::OnValidateTaskFinished(
    bool result,
    mojom::PinErrorPtr error) {
  if (!result) {
    AddOrExecute(std::make_unique<IntentData>(current_->token, Operation::kAdd,
                                              current_->service));
  }
  current_.reset();
  CheckQueue();
}

void BraveWalletAutoPinService::SetAutoPinEnabled(bool enabled) {
  pref_service_->SetBoolean(kAutoPinEnabled, enabled);
  Restore();
}

bool BraveWalletAutoPinService::IsAutoPinEnabled() {
  return pref_service_->GetBoolean(kAutoPinEnabled);
}

void BraveWalletAutoPinService::IsAutoPinEnabled(
    IsAutoPinEnabledCallback callback) {
  std::move(callback).Run(IsAutoPinEnabled());
}

}  // namespace brave_wallet
