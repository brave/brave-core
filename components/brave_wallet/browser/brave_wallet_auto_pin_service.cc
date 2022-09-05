#include "brave/components/brave_wallet/browser/brave_wallet_auto_pin_service.h"

#include "brave/components/brave_wallet/browser/pref_names.h"

namespace brave_wallet {

namespace {
const char kLocalServiceName[] = "local";
}

IntentData::IntentData(const BlockchainTokenPtr& token,
                       Operation operation,
                       absl::optional<std::string> service)
    : token(token.Clone()), operation(operation), service(service) {}

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

void BraveWalletAutoPinService::OnTokenAdded(BlockchainTokenPtr token) {
  if (!IsAutoPinEnabled()) {
    return;
  }
  PostPinToken(std::move(token), base::OnceCallback<void(bool)>());
}

void BraveWalletAutoPinService::OnTokenRemoved(BlockchainTokenPtr token) {
  PostUnpinToken(std::move(token), base::OnceCallback<void(bool)>());
}

void BraveWalletAutoPinService::Restore() {
  brave_wallet_service_->GetUserAssets(
      mojom::kMainnetChainId, mojom::CoinType::ETH,
      base::BindOnce(&BraveWalletAutoPinService::OnTokenListResolved,
                     base::Unretained(this)));
}

void BraveWalletAutoPinService::OnTokenListResolved(
    std::vector<BlockchainTokenPtr> token_list) {
  LOG(ERROR) << "XXXZZZ token list resolved " << token_list.size();
  bool autopin_enabled = IsAutoPinEnabled();
  std::vector<absl::optional<std::string>> services = GetServicesToPin();
  for (const auto& token : token_list) {
    if (!token->is_erc721) {
      continue;
    }
    for (const auto& service : services) {
      mojom::TokenPinStatusCode status =
          brave_wallet_pin_service_->GetTokenStatus(service, token);
      if (status == mojom::TokenPinStatusCode::STATUS_PINNING_FAILED ||
          status == mojom::TokenPinStatusCode::STATUS_PINNING_IN_PROGRESS ||
          status == mojom::TokenPinStatusCode::STATUS_PINNING_PENDING) {
        AddOrExecute(
            std::make_unique<IntentData>(token, Operation::ADD, service));
      } else if (status == mojom::TokenPinStatusCode::STATUS_UNPINNING_FAILED ||
                 status ==
                     mojom::TokenPinStatusCode::STATUS_UNPINNING_IN_PROGRESS ||
                 status ==
                     mojom::TokenPinStatusCode::STATUS_UNPINNING_PENDING) {
        AddOrExecute(
            std::make_unique<IntentData>(token, Operation::DELETE, service));
      } else if (status == mojom::TokenPinStatusCode::STATUS_NOT_PINNED &&
                 autopin_enabled) {
        AddOrExecute(
            std::make_unique<IntentData>(token, Operation::ADD, service));
      } else if (status == mojom::TokenPinStatusCode::STATUS_PINNED) {
        auto t1 =
            brave_wallet_pin_service_->GetLastValidateTime(service, token);
        if (!t1 || (base::Time::Now() - t1.value()) > base::Days(1)) {
          AddOrExecute(std::make_unique<IntentData>(token, Operation::VALIDATE,
                                                    service));
        }
      }
    }
  }
  CheckQueue();
}

void BraveWalletAutoPinService::SetServices(
    const std::vector<absl::optional<std::string>>& services) {
  base::Value::List list;
  for (const auto& service : services) {
    list.Append(base::Value(service.value_or(kLocalServiceName)));
  }
  pref_service_->SetList(kSelectedPinServices, std::move(list));
  Restore();
}

void BraveWalletAutoPinService::PostPinToken(BlockchainTokenPtr token, PostPinTokenCallback callback) {
  std::vector<absl::optional<std::string>> services = GetServicesToPin();
  for (const auto& service : services) {
    queue_.push_back(std::make_unique<IntentData>(token, Operation::ADD, service));
  }
  CheckQueue();
}

void BraveWalletAutoPinService::PostUnpinToken(BlockchainTokenPtr token, PostPinTokenCallback callback) {
  std::vector<absl::optional<std::string>> services = GetKnownServices();
  for (const auto& service : services) {
    queue_.push_back(
        std::make_unique<IntentData>(token, Operation::DELETE, service));
  }
  CheckQueue();
}

void BraveWalletAutoPinService::ValidateToken(
    const std::unique_ptr<IntentData>& data) {
  LOG(ERROR) << "XXXZZZ validate token";
  brave_wallet_pin_service_->Validate(
      data->token->Clone(), data->service,
      base::BindOnce(&BraveWalletAutoPinService::OnTaskFinished,
                     base::Unretained(this)));
}

void BraveWalletAutoPinService::PinToken(
    const std::unique_ptr<IntentData>& data) {
  LOG(ERROR) << "XXXZZZ pin token";

  brave_wallet_pin_service_->AddPin(
      data->token->Clone(), data->service,
      base::BindOnce(&BraveWalletAutoPinService::OnTaskFinished,
                     base::Unretained(this)));
}

void BraveWalletAutoPinService::UnpinToken(
    const std::unique_ptr<IntentData>& data) {
    LOG(ERROR) << "XXXZZZ unpin token";

  brave_wallet_pin_service_->RemovePin(
      data->token->Clone(), data->service,
      base::BindOnce(&BraveWalletAutoPinService::OnTaskFinished,
                     base::Unretained(this)));
}

void BraveWalletAutoPinService::AddOrExecute(std::unique_ptr<IntentData> data) {
  LOG(ERROR) << "XXXZZZ AddOrExecute " << data->token->contract_address << " " << data->service.value_or("local") << " " << data->operation;
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
  if (data->operation == Operation::ADD) {
    brave_wallet_pin_service_->MarkAsPendingForPinning(data->token,
                                                       data->service);
  } else if (data->operation == Operation::DELETE) {
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
                     base::Unretained(this), std::move(data)),
      base::Minutes(1 * multiply));
}

void BraveWalletAutoPinService::CheckQueue() {
  if (queue_.empty() || current_) {
    return;
  }

  current_ = std::move(queue_.front());
  queue_.pop_front();

  if (current_->operation == Operation::ADD) {
    PinToken(current_);
  } else if (current_->operation == Operation::DELETE) {
    UnpinToken(current_);
  } else if (current_->operation == Operation::VALIDATE) {
    ValidateToken(current_);
  }
}

void BraveWalletAutoPinService::OnTaskFinished(bool result,
                                               mojom::PinErrorPtr error) {
  CHECK(current_);
  LOG(ERROR) << "XXXZZZ OnTaskFinished " << current_->token->contract_address << " " << result;
  if (!result) {
    PostRetry(std::move(current_));
  }
  current_.reset();
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

void BraveWalletAutoPinService::GetServices(GetServicesCallback callback) {
  std::move(callback).Run(GetServicesToPin());
}

std::vector<absl::optional<std::string>>
BraveWalletAutoPinService::GetServicesToPin() {
  std::vector<absl::optional<std::string>> result;
  //  const base::Value::List& list =
  //  pref_service_->GetList(kSelectedPinServices); for (const auto& item :
  //  list) {
  //    auto* value = item.GetIfString();
  //    DCHECK(value) << "Wrong prefs structure";
  //    if (value) {
  //        if (*value == kLocalServiceName) {
  //          result.push_back(absl::nullopt);
  //        } else {
  //          result.push_back(*value);
  //        }
  //    }
  //  }
  result.push_back(absl::nullopt);
  return result;
}

std::vector<absl::optional<std::string>>
BraveWalletAutoPinService::GetKnownServices() {
  std::vector<absl::optional<std::string>> result;
  //  const base::Value::List& list =
  //  pref_service_->GetList(kSelectedPinServices); for (const auto& item :
  //  list) {
  //    auto* value = item.GetIfString();
  //    DCHECK(value) << "Wrong prefs structure";
  //    if (value) {
  //        if (*value == kLocalServiceName) {
  //          result.push_back(absl::nullopt);
  //        } else {
  //          result.push_back(*value);
  //        }
  //    }
  //  }
  result.push_back(absl::nullopt);
  return result;
}

}  // namespace brave_wallet
