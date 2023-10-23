/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"

#include <set>
#include <utility>

#include "brave/components/brave_wallet/common/common_utils.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

class GetTransparentUtxosContext
    : public base::RefCountedThreadSafe<GetTransparentUtxosContext> {
 public:
  using GetUtxosCallback = ZCashWalletService::GetUtxosCallback;

  std::set<std::string> addresses;
  ZCashWalletService::UtxoMap utxos;
  absl::optional<std::string> error;
  GetUtxosCallback callback;

  bool ShouldRespond() { return callback && (error || addresses.empty()); }

  void SetError(const std::string& error_string) { error = error_string; }

 protected:
  friend class base::RefCountedThreadSafe<GetTransparentUtxosContext>;
  virtual ~GetTransparentUtxosContext() = default;
};

class CreateTransparentTransactionTask {
 public:
  using UtxoMap = ZCashWalletService::UtxoMap;
  using CreateTransactionCallback =
      ZCashWalletService::CreateTransactionCallback;

  CreateTransparentTransactionTask(ZCashWalletService* zcash_wallet_service,
                                   const std::string& chain_id,
                                   const mojom::AccountIdPtr& account_id,
                                   const std::string& address_to,
                                   uint64_t amount,
                                   CreateTransactionCallback callback);

  void ScheduleWorkOnTask();

 private:
  bool IsTestnet() { return chain_id_ == mojom::kZCashTestnet; }
  void WorkOnTask();

  void SetError(const std::string& error_string) { error_ = error_string; }

  bool PickInputs();
  bool PrepareOutputs();

  void OnGetChainHeight(base::expected<zcash::BlockID, std::string> result);
  void OnGetUtxos(
      base::expected<ZCashWalletService::UtxoMap, std::string> utxo_map);

  raw_ptr<ZCashWalletService> zcash_wallet_service_;  // Owns `this`.
  std::string chain_id_;
  mojom::AccountIdPtr account_id_;
  CreateTransactionCallback callback_;

  absl::optional<uint32_t> chain_height_;
  ZCashWalletService::UtxoMap utxo_map_;

  absl::optional<std::string> error_;
  ZCashTransaction transaction_;

  base::WeakPtrFactory<CreateTransparentTransactionTask> weak_ptr_factory_{
      this};
};

CreateTransparentTransactionTask::CreateTransparentTransactionTask(
    ZCashWalletService* zcash_wallet_service,
    const std::string& chain_id,
    const mojom::AccountIdPtr& account_id,
    const std::string& address_to,
    uint64_t amount,
    CreateTransactionCallback callback)
    : zcash_wallet_service_(zcash_wallet_service),
      chain_id_(chain_id),
      account_id_(account_id.Clone()),
      callback_(std::move(callback)) {
  transaction_.set_to(address_to);
  transaction_.set_amount(amount);
}

void CreateTransparentTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&CreateTransparentTransactionTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void CreateTransparentTransactionTask::WorkOnTask() {
  if (!callback_) {
    return;
  }

  if (error_) {
    std::move(callback_).Run(base::unexpected(*error_));
    zcash_wallet_service_->CreateTransactionTaskDone(this);
    return;
  }

  if (!chain_height_) {
    zcash_wallet_service_->zcash_rpc()->GetLatestBlock(
        chain_id_,
        base::BindOnce(&CreateTransparentTransactionTask::OnGetChainHeight,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (utxo_map_.empty()) {
    zcash_wallet_service_->GetUtxos(
        chain_id_, account_id_.Clone(),
        base::BindOnce(&CreateTransparentTransactionTask::OnGetUtxos,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  // TODO(cypt4): fetch fee estimations, calculate transaction size and set
  // transaction fee
  transaction_.set_fee(2000);

  // TODO(cypt4): random shift locktime
  // https://github.com/bitcoin/bitcoin/blob/v24.0/src/wallet/spend.cpp#L739-L747
  transaction_.set_locktime(chain_height_.value());

  if (!PickInputs()) {
    SetError("Couldn't pick transaction inputs");
    ScheduleWorkOnTask();
    return;
  }

  if (!PrepareOutputs()) {
    SetError("Couldn't prepare outputs");
    ScheduleWorkOnTask();
    return;
  }

  std::move(callback_).Run(base::ok(std::move(transaction_)));
  zcash_wallet_service_->CreateTransactionTaskDone(this);
}

void CreateTransparentTransactionTask::OnGetChainHeight(
    base::expected<zcash::BlockID, std::string> result) {
  if (!result.has_value()) {
    SetError(std::move(result).error());
    WorkOnTask();
    return;
  }

  chain_height_ = result.value().height();
  WorkOnTask();
}

void CreateTransparentTransactionTask::OnGetUtxos(
    base::expected<UtxoMap, std::string> utxo_map) {
  if (!utxo_map.has_value()) {
    SetError(std::move(utxo_map).error());
    WorkOnTask();
    return;
  }

  utxo_map_ = std::move(utxo_map.value());
  WorkOnTask();
}

bool CreateTransparentTransactionTask::PickInputs() {
  bool done = false;

  // TODO(apaymyshev): This just picks ouputs one by one and stops when picked
  // amount is GE to send amount plus fee. Needs something better than such
  // greedy strategy.
  std::vector<ZCashTransaction::TxInput> all_inputs;
  for (const auto& item : utxo_map_) {
    for (const auto& utxo : item.second) {
      if (auto input =
              ZCashTransaction::TxInput::FromRpcUtxo(item.first, utxo)) {
        all_inputs.emplace_back(std::move(*input));
      }
    }
  }

  base::ranges::sort(all_inputs, [](auto& input1, auto& input2) {
    return input1.utxo_value < input2.utxo_value;
  });

  for (auto& input : all_inputs) {
    transaction_.inputs().push_back(std::move(input));
    if (transaction_.TotalInputsAmount() >=
        transaction_.amount() + transaction_.fee()) {
      done = true;
    }

    if (done) {
      break;
    }
  }

  DCHECK(!transaction_.inputs().empty());
  return done;
}

bool CreateTransparentTransactionTask::PrepareOutputs() {
  auto& target_output = transaction_.outputs().emplace_back();
  target_output.address = transaction_.to();
  target_output.amount = transaction_.amount();
  //  if (!OutputAddressSupported(target_output.address, IsTestnet())) {
  //    return false;
  //  }

  CHECK_GE(transaction_.TotalInputsAmount(),
           transaction_.amount() + transaction_.fee());
  uint64_t change_amount = transaction_.TotalInputsAmount() -
                           transaction_.amount() - transaction_.fee();
  if (change_amount == 0) {
    return true;
  }

  // TODO(cypt4): should always pick new change address.
  const auto& change_address =
      zcash_wallet_service_->GetUnusedChangeAddress(*account_id_);
  if (!change_address) {
    return false;
  }

  // CHECK(OutputAddressSupported(*change_address, IsTestnet()));
  auto& change_output = transaction_.outputs().emplace_back();
  change_output.address = *change_address;
  change_output.amount = change_amount;
  return true;
}

ZCashWalletService::ZCashWalletService(
    KeyringService* keyring_service,
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : keyring_service_(keyring_service),
      zcash_rpc_(
          std::make_unique<zcash_rpc::ZCashRpc>(prefs, url_loader_factory)) {
  zcash_rpc_ = std::make_unique<zcash_rpc::ZCashRpc>(prefs, url_loader_factory);
}

mojo::PendingRemote<mojom::ZCashWalletService>
ZCashWalletService::MakeRemote() {
  mojo::PendingRemote<mojom::ZCashWalletService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void ZCashWalletService::Bind(
    mojo::PendingReceiver<mojom::ZCashWalletService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

ZCashWalletService::~ZCashWalletService() = default;

absl::optional<std::string> ZCashWalletService::GetUnusedChangeAddress(
    const mojom::AccountId& account_id) {
  CHECK(IsZCashAccount(account_id));
  // TODO(cypt4): this always returns first change address. Should return
  // first unused change address.
  return keyring_service_->GetZCashAddress(
      account_id, mojom::ZCashKeyId(account_id.bitcoin_account_index, 1, 0));
}

void ZCashWalletService::GetBalance(const std::string& chain_id,
                                    mojom::AccountIdPtr account_id,
                                    GetBalanceCallback callback) {
  GetUtxos(chain_id, std::move(account_id),
           base::BindOnce(&ZCashWalletService::OnUtxosResolvedForBalance,
                          weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ZCashWalletService::GetReceiverAddress(
    mojom::AccountIdPtr account_id,
    GetReceiverAddressCallback callback) {
  auto id = mojom::ZCashKeyId::New(account_id->bitcoin_account_index, 0, 0);
  auto str_addr = keyring_service_->GetZCashAddress(*account_id, *id);
  if (!str_addr) {
    std::move(callback).Run(nullptr, "Failed to retreive new receiver address");
  }
  // TODO(cypt4): Return unused receiver address
  std::move(callback).Run(mojom::ZCashAddress::New(*str_addr, std::move(id)),
                          absl::nullopt);
}

void ZCashWalletService::GetUtxos(const std::string& chain_id,
                                  mojom::AccountIdPtr account_id,
                                  GetUtxosCallback callback) {
  if (!IsZCashNetwork(chain_id)) {
    // Desktop frontend sometimes does that.
    std::move(callback).Run(
        base::unexpected("Invalid bitcoin chain id " + chain_id));
    return;
  }

  const auto& addresses = keyring_service_->GetZCashAddresses(*account_id);
  if (!addresses) {
    std::move(callback).Run(base::unexpected("Couldn't get balance"));
    return;
  }

  auto context = base::MakeRefCounted<GetTransparentUtxosContext>();
  context->callback = std::move(callback);
  for (const auto& address : addresses.value()) {
    context->addresses.insert(address.first);
  }

  for (const auto& address : context->addresses) {
    zcash_rpc_->GetUtxoList(
        chain_id, {address},
        base::BindOnce(&ZCashWalletService::OnGetUtxos,
                       weak_ptr_factory_.GetWeakPtr(), context, address));
  }
}

void ZCashWalletService::SignAndPostTransaction(
    const std::string& chain_id,
    const mojom::AccountIdPtr& account_id,
    ZCashTransaction zcash_transaction,
    SignAndPostTransactionCallback callback) {}

void ZCashWalletService::OnGetUtxos(
    scoped_refptr<GetTransparentUtxosContext> context,
    const std::string& address,
    base::expected<std::vector<zcash::ZCashUtxo>, std::string> result) {
  DCHECK(context->addresses.contains(address));
  DCHECK(!context->utxos.contains(address));

  if (!result.has_value()) {
    context->SetError(result.error());
    WorkOnGetUtxos(std::move(context));
    return;
  }

  context->addresses.erase(address);
  context->utxos[address] = result.value();

  WorkOnGetUtxos(std::move(context));
}

void ZCashWalletService::WorkOnGetUtxos(
    scoped_refptr<GetTransparentUtxosContext> context) {
  if (!context->ShouldRespond()) {
    return;
  }

  if (context->error) {
    std::move(context->callback)
        .Run(base::unexpected(std::move(*context->error)));
    return;
  }

  std::move(context->callback).Run(std::move(context->utxos));
}

void ZCashWalletService::OnUtxosResolvedForBalance(
    GetBalanceCallback initial_callback,
    base::expected<UtxoMap, std::string> utxos) {
  if (!utxos.has_value()) {
    std::move(initial_callback).Run(nullptr, utxos.error());
    return;
  }

  auto result = mojom::ZCashBalance::New();
  result->total_balance = 0;
  for (const auto& by_addr : utxos.value()) {
    uint64_t balance_by_addr = 0;
    for (const auto& by_utxo : by_addr.second) {
      balance_by_addr += by_utxo.valuezat();
    }
    result->total_balance += balance_by_addr;
    result->balances[by_addr.first] = balance_by_addr;
  }
  std::move(initial_callback).Run(std::move(result), absl::nullopt);
}

void ZCashWalletService::CreateTransaction(const std::string& chain_id,
                                           mojom::AccountIdPtr account_id,
                                           const std::string& address_to,
                                           uint64_t amount,
                                           CreateTransactionCallback callback) {
  auto& task = create_transaction_tasks_.emplace_back(
      std::make_unique<CreateTransparentTransactionTask>(
          this, chain_id, account_id, address_to, amount, std::move(callback)));
  task->ScheduleWorkOnTask();
}

void ZCashWalletService::GetTransactionStatus(
    const std::string& chain_id,
    const std::string& tx_hash,
    GetTransactionStatusCallback callback) {
  zcash_rpc_->GetTransaction(
      chain_id, tx_hash,
      base::BindOnce(&ZCashWalletService::OnTransactionResolvedForStatus,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ZCashWalletService::OnTransactionResolvedForStatus(
    GetTransactionStatusCallback callback,
    base::expected<zcash::RawTransaction, std::string> result) {
  if (!result.has_value()) {
    std::move(callback).Run(base::unexpected(result.error()));
    return;
  }

  std::move(callback).Run(result.value().height() + 1 != 0);
}

void ZCashWalletService::CreateTransactionTaskDone(
    CreateTransparentTransactionTask* task) {
  CHECK(create_transaction_tasks_.remove_if(
      [task](auto& item) { return item.get() == task; }));
}

zcash_rpc::ZCashRpc* ZCashWalletService::zcash_rpc() {
  return zcash_rpc_.get();
}

}  // namespace brave_wallet
