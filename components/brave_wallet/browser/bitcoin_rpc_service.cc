/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin_rpc_service.h"
#include <stdint.h>

#include <deque>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece_forward.h"
#include "base/sys_byteorder.h"
#include "brave/components/brave_wallet/browser/internal/hd_key.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "components/prefs/pref_service.h"
#include "crypto/sha2.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

#pragma clang optimize off

namespace {
constexpr uint32_t kSigHashAll = 1;

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("bitcoin_rpc_service", R"(
      semantics {
        sender: "Bitcoin RPC Service"
        description:
          "This service is used to communicate with Bitcoin nodes "
          "on behalf of the user interacting with the native Brave wallet."
        trigger:
          "Triggered by uses of the native Brave wallet."
        data:
          "Bitcoin JSON RPC response bodies."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature on chrome://flags."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

const GURL MakeGetChainHeightUrl(const GURL& base_url) {
  GURL::Replacements replacements;
  const std::string path =
      base_url.path() + base::JoinString({"blocks", "tip", "height"}, "/");
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

const GURL MakeListUtxoUrl(const GURL& base_url, const std::string& address) {
  GURL::Replacements replacements;
  const std::string path =
      base_url.path() + base::JoinString({"address", address, "utxo"}, "/");
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

const GURL MakeFetchTransactionUrl(const GURL& base_url,
                                   const std::string& txid) {
  GURL::Replacements replacements;
  const std::string path =
      base_url.path() + base::JoinString({"tx", txid}, "/");
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

const GURL MakePostTransactionUrl(const GURL& base_url) {
  GURL::Replacements replacements;
  const std::string path = base_url.path() + "tx";
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

GURL BaseRpcUrl(const std::string& keyring_id) {
  return GURL(keyring_id == brave_wallet::mojom::kBitcoinKeyringId
                  ? "https://blockstream.info/api/"
                  : "https://blockstream.info/testnet/api/");
}

std::vector<uint8_t> DoubleHash(base::span<const uint8_t> data) {
  auto double_hash = crypto::SHA256Hash(crypto::SHA256Hash(data));
  return {double_hash.begin(), double_hash.end()};
}

void PushAsLE(uint8_t i, std::vector<uint8_t>& to) {
  to.push_back(i);
}

void PushAsLE(uint16_t i, std::vector<uint8_t>& to) {
  i = base::ByteSwapToLE16(i);
  base::span<uint8_t> data_to_insert(reinterpret_cast<uint8_t*>(&i), sizeof(i));
  to.insert(to.end(), data_to_insert.begin(), data_to_insert.end());
}

void PushAsLE(uint32_t i, std::vector<uint8_t>& to) {
  i = base::ByteSwapToLE32(i);
  base::span<uint8_t> data_to_insert(reinterpret_cast<uint8_t*>(&i), sizeof(i));
  to.insert(to.end(), data_to_insert.begin(), data_to_insert.end());
}

void PushAsLE(uint64_t i, std::vector<uint8_t>& to) {
  i = base::ByteSwapToLE64(i);
  base::span<uint8_t> data_to_insert(reinterpret_cast<uint8_t*>(&i), sizeof(i));
  to.insert(to.end(), data_to_insert.begin(), data_to_insert.end());
}

void PushAsLE(const std::vector<uint8_t>& v, std::vector<uint8_t>& to) {
  to.insert(to.end(), v.rbegin(), v.rend());
}

void PushVarInt(uint64_t i, std::vector<uint8_t>& to) {
  if (i < 0xfd) {
    PushAsLE(uint8_t(i), to);
  } else if (i < 0xffff) {
    PushAsLE(uint8_t(0xfd), to);
    PushAsLE(uint16_t(i), to);
  } else if (i < 0xffffffff) {
    PushAsLE(uint8_t(0xfe), to);
    PushAsLE(uint32_t(i), to);
  } else {
    PushAsLE(uint8_t(0xff), to);
    PushAsLE(uint64_t(i), to);
  }
}

void PushVarSizeVector(const std::vector<uint8_t>& v,
                       std::vector<uint8_t>& to) {
  PushVarInt(v.size(), to);
  to.insert(to.end(), v.begin(), v.end());
}

absl::optional<std::string> ConvertPlainIntToJsonArray(
    const std::string& json) {
  return "[" + json + "]";
}

absl::optional<std::string> ConvertPlainStringToJsonArray(
    const std::string& json) {
  return "[\"" + json + "\"]";
}

}  // namespace

namespace brave_wallet {

namespace {
mojom::BitcoinUnspentOutputPtr BitcoinUnspentOutputFromValue(
    const base::Value::Dict* dict) {
  if (!dict) {
    return nullptr;
  }

  auto result = mojom::BitcoinUnspentOutput::New();
  if (auto* txid = dict->FindString("txid")) {
    result->txid = *txid;
  } else {
    return nullptr;
  }

  if (!base::HexStringToBytes(result->txid, &result->txid_bin)) {
    return nullptr;
  }

  if (auto vout = dict->FindInt("vout")) {
    result->vout = *vout;
  } else {
    return nullptr;
  }

  // TOOD(apaymyshev): int64
  if (auto value = dict->FindInt("value")) {
    result->value = *value;
  } else {
    return nullptr;
  }

  return result;
}
}  // namespace

struct GetBitcoinAccountInfoContext
    : base::RefCounted<GetBitcoinAccountInfoContext> {
 public:
  GetBitcoinAccountInfoContext() = default;

  std::set<mojom::BitcoinAddressPtr> pending_addresses;
  mojom::BitcoinAccountInfoPtr account_info;
  mojom::BitcoinRpcService::GetBitcoinAccountInfoCallback callback;

 private:
  friend class base::RefCounted<GetBitcoinAccountInfoContext>;
  ~GetBitcoinAccountInfoContext() = default;
};

struct BitcoinInput {
  mojom::BitcoinUnspentOutputPtr unspent_output;
  mojom::BitcoinAddressPtr address;
  base::Value transaction;
  uint32_t n_sequence = 0xfffffffd;  // TODO(apaymyshev): or 0xffffffff ?
  std::vector<uint8_t> pubkey;
  std::vector<uint8_t> signature;
};

struct BitcoinOutput {
  std::string address;
  std::vector<uint8_t> pubkey_hash;
  std::vector<uint8_t> script_pubkey;
  uint64_t amount = 0;
};

struct SendToContext {
 public:
  std::string keyring_id;
  uint32_t account_index;
  std::string address_to;
  uint64_t amount = 0;
  uint64_t fee = 0;
  uint64_t amount_picked = 0;
  mojom::BitcoinAccountInfoPtr account_info;
  std::vector<BitcoinInput> inputs;
  std::vector<BitcoinOutput> outputs;
  uint32_t locktime = 0;

  std::vector<uint8_t> transaction;

  mojom::BitcoinRpcService::SendToCallback callback;

  bool IsTestnet() { return keyring_id == mojom::kBitcoinTestnetKeyringId; }
  bool InputsPicked() { return !inputs.empty(); }
  bool OutputsPrepared() { return !outputs.empty(); }
  bool InputTransactionsReady() {
    return base::ranges::all_of(
        inputs, [](auto& input) { return input.transaction.is_dict(); });
  }
  bool PubkeysReady() {
    return base::ranges::all_of(
        inputs, [](auto& input) { return !input.pubkey.empty(); });
  }
  bool SignaturesReady() {
    return base::ranges::all_of(
        inputs, [](auto& input) { return !input.signature.empty(); });
  }
  bool TransactionReady() { return !transaction.empty(); }

  void ReplyFailure() { std::move(callback).Run("", ""); }
};

BitcoinRpcService::BitcoinRpcService(
    KeyringService* keyring_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* prefs,
    PrefService* local_state_prefs)
    : keyring_service_(keyring_service),
      api_request_helper_(new APIRequestHelper(GetNetworkTrafficAnnotationTag(),
                                               url_loader_factory)),
      prefs_(prefs),
      local_state_prefs_(local_state_prefs) {}

BitcoinRpcService::~BitcoinRpcService() = default;

mojo::PendingRemote<mojom::BitcoinRpcService> BitcoinRpcService::MakeRemote() {
  mojo::PendingRemote<mojom::BitcoinRpcService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void BitcoinRpcService::Bind(
    mojo::PendingReceiver<mojom::BitcoinRpcService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void BitcoinRpcService::GetChainHeight(const std::string& keyring_id,
                                       GetChainHeightCallback callback) {
  if (keyring_id != mojom::kBitcoinKeyringId &&
      keyring_id != mojom::kBitcoinTestnetKeyringId) {
    return;
  }
  GURL network_url = BaseRpcUrl(keyring_id);

  auto internal_callback =
      base::BindOnce(&BitcoinRpcService::OnGetChainHeight,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  // Response comes as a plain integer which is not accepted by json sanitizer.
  // Wrap response into json array.
  auto conversion_callback = base::BindOnce(&ConvertPlainIntToJsonArray);
  RequestInternal(true, MakeGetChainHeightUrl(network_url),
                  std::move(internal_callback), std::move(conversion_callback));
}

void BitcoinRpcService::GetUtxoList(const std::string& keyring_id,
                                    const std::string& address,
                                    GetUtxoListCallback callback) {
  if (!prefs_ || !local_state_prefs_) {
    return;
  }

  if (keyring_id != mojom::kBitcoinKeyringId &&
      keyring_id != mojom::kBitcoinTestnetKeyringId) {
    return;
  }

  GURL network_url = BaseRpcUrl(keyring_id);

  auto internal_callback =
      base::BindOnce(&BitcoinRpcService::OnGetUtxoList,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(true, MakeListUtxoUrl(network_url, address),
                  std::move(internal_callback));
}

void BitcoinRpcService::GetBitcoinAccountInfo(
    const std::string& keyring_id,
    uint32_t account_index,
    GetBitcoinAccountInfoCallback callback) {
  if (keyring_id != mojom::kBitcoinKeyringId &&
      keyring_id != mojom::kBitcoinTestnetKeyringId) {
    std::move(callback).Run(nullptr);
    return;
  }

  auto addresses =
      keyring_service_->GetBitcoinAddressesSync(keyring_id, account_index);
  if (addresses.empty()) {
    NOTREACHED();
    std::move(callback).Run(nullptr);
    return;
  }

  auto context = base::MakeRefCounted<GetBitcoinAccountInfoContext>();
  context->account_info = mojom::BitcoinAccountInfo::New();

  for (auto& address : addresses) {
    auto& info = context->account_info->address_infos.emplace_back(
        mojom::BitcoinAddressInfo::New());
    info->address = address->Clone();

    context->pending_addresses.insert(address->Clone());
  }

  // for (auto& address : addresses.second) {
  //   auto& info = context->account_info->address_infos.emplace_back(
  //       mojom::BitcoinAddressInfo::New());
  //   info->address = address;
  //   info->change = true;

  //   context->pending_addresses.insert(address);
  // }

  context->callback = std::move(callback);

  for (auto& address : context->pending_addresses) {
    auto request_url =
        MakeListUtxoUrl(BaseRpcUrl(keyring_id), address->address);
    auto internal_callback = base::BindOnce(
        &BitcoinRpcService::OnGetUtxoListForBitcoinAccountInfo,
        weak_ptr_factory_.GetWeakPtr(), context, address->Clone());
    RequestInternal(true, request_url, std::move(internal_callback));
  }
}

void BitcoinRpcService::SendTo(const std::string& keyring_id,
                               uint32_t account_index,
                               const std::string& address_to,
                               uint64_t amount,
                               uint64_t fee,
                               SendToCallback callback) {
  if (keyring_id != mojom::kBitcoinKeyringId &&
      keyring_id != mojom::kBitcoinTestnetKeyringId) {
    std::move(callback).Run("", "");
    return;
  }

  auto context = std::make_unique<SendToContext>();
  context->keyring_id = keyring_id;
  context->account_index = account_index;
  context->address_to = address_to;
  context->amount = amount;
  context->fee = fee;
  context->callback = std::move(callback);

  // context->address_to = "tb1q6sudmjpyga5uf7gtkglk97msq34j5x9wccheft";
  // context->amount = 7890;
  // context->fee = 110;
  // context->locktime = 2425275;

  WorkOnSendTo(std::move(context));
}

void BitcoinRpcService::OnGetChainHeightForSendTo(
    std::unique_ptr<SendToContext> context,
    uint32_t height) {
  if (!height) {
    return context->ReplyFailure();
  }

  // TODO(apymyshev): random shift locktime
  // https://github.com/bitcoin/bitcoin/blob/df73c23f5fac031cc9b2ec06a74275db5ea322e3/src/wallet/wallet.cpp#L2595-L2600
  context->locktime = height;

  WorkOnSendTo(std::move(context));
}

void BitcoinRpcService::OnGetBitcoinAccountInfoForSendTo(
    std::unique_ptr<SendToContext> context,
    mojom::BitcoinAccountInfoPtr account_info) {
  if (!account_info) {
    return context->ReplyFailure();
  }
  context->account_info = std::move(account_info);
  WorkOnSendTo(std::move(context));
}

bool BitcoinRpcService::PickInputs(SendToContext& context) {
  context.amount_picked = 0;
  bool done = false;

  // TODO(apaymyshev): needs something better than greedy strategy.
  for (auto& address_info : context.account_info->address_infos) {
    for (auto& utxo : address_info->utxo_list) {
      context.amount_picked += utxo->value;
      auto& input = context.inputs.emplace_back();
      input.unspent_output = utxo->Clone();
      input.address = address_info->address->Clone();
      if (context.amount_picked >= context.amount + context.fee) {
        done = true;
        break;
      }
    }
    if (done) {
      break;
    }
  }

  DCHECK(!context.inputs.empty());
  return done;
}

bool BitcoinRpcService::PrepareOutputs(SendToContext& context) {
  auto& target_output = context.outputs.emplace_back();
  target_output.address = context.address_to;
  target_output.amount = context.amount;
  if (auto pubkey_hash =
          DecodeBitcoinAddress(target_output.address, context.IsTestnet())) {
    target_output.pubkey_hash = std::move(*pubkey_hash);
    CHECK_EQ(target_output.pubkey_hash.size(), 20u);

    PushAsLE(uint8_t(0), target_output.script_pubkey);  // OP_0
    PushAsLE(uint8_t(target_output.pubkey_hash.size()),
             target_output.script_pubkey);
    target_output.script_pubkey.insert(target_output.script_pubkey.end(),
                                       target_output.pubkey_hash.begin(),
                                       target_output.pubkey_hash.end());
  } else {
    return false;
  }

  CHECK_GE(context.amount_picked, context.amount + context.fee);
  if (context.amount_picked == context.amount + context.fee) {
    return true;
  }

  std::string change_address;
  // TODO(apaymyshev): should always pick new change address.
  for (auto& address_info : context.account_info->address_infos) {
    if (address_info->address->id->change) {
      change_address = address_info->address->address;
      break;
    }
  }
  if (change_address.empty()) {
    return false;
  }

  auto& change_output = context.outputs.emplace_back();
  change_output.address = change_address;
  change_output.amount = context.amount_picked - context.amount - context.fee;
  if (auto pubkey_hash =
          DecodeBitcoinAddress(change_output.address, context.IsTestnet())) {
    change_output.pubkey_hash = std::move(*pubkey_hash);
    CHECK_EQ(change_output.pubkey_hash.size(), 20u);

    PushAsLE(uint8_t(0), change_output.script_pubkey);   // OP_0
    PushAsLE(uint8_t(change_output.pubkey_hash.size()),  // OP_14
             change_output.script_pubkey);
    change_output.script_pubkey.insert(change_output.script_pubkey.end(),
                                       change_output.pubkey_hash.begin(),
                                       change_output.pubkey_hash.end());
  } else {
    return false;
  }

  return true;
}

void BitcoinRpcService::FetchInputTransactions(
    std::unique_ptr<SendToContext> context) {
  DCHECK(!context->InputTransactionsReady());

  for (auto& input : context->inputs) {
    if (input.transaction.is_none()) {
      auto keyring_id = context->keyring_id;
      auto txid = input.unspent_output->txid;
      FetchTransaction(
          keyring_id, txid,
          base::BindOnce(&BitcoinRpcService::OnFetchTransactionForSendTo,
                         weak_ptr_factory_.GetWeakPtr(), std::move(context),
                         txid));
      return;
    }
  }

  NOTREACHED();
}

void BitcoinRpcService::OnFetchTransactionForSendTo(
    std::unique_ptr<SendToContext> context,
    std::string txid,
    base::Value transaction) {
  if (transaction.is_none()) {
    return context->ReplyFailure();
  }

  bool unspent_output_found = false;
  for (auto& input : context->inputs) {
    if (input.unspent_output->txid == txid) {
      input.transaction = transaction.Clone();
      unspent_output_found = true;
    }
  }

  DCHECK(unspent_output_found);

  WorkOnSendTo(std::move(context));
}

void PushHashPrevouts(SendToContext& context, std::vector<uint8_t>& to) {
  std::vector<uint8_t> data;
  for (auto& input : context.inputs) {
    PushAsLE(input.unspent_output->txid_bin, data);
    PushAsLE(input.unspent_output->vout, data);
  }

  auto double_hash = DoubleHash(data);
  to.insert(to.end(), double_hash.begin(), double_hash.end());
}

void PushHashSequence(SendToContext& context, std::vector<uint8_t>& to) {
  std::vector<uint8_t> data;
  for (auto& input : context.inputs) {
    PushAsLE(input.n_sequence, data);
  }

  auto double_hash = DoubleHash(data);
  to.insert(to.end(), double_hash.begin(), double_hash.end());
}

// TODO(apaymyshev): calculate from input.pubkey.
std::vector<uint8_t> GetScriptPubkeyHash(SendToContext& context,
                                         uint32_t input_index) {
  auto& input = context.inputs[input_index];

  base::Value& transaction = input.transaction;
  uint32_t vout_index = input.unspent_output->vout;

  auto* transaction_dict = transaction.GetIfDict();
  CHECK(transaction_dict);
  auto* vout_list = transaction_dict->FindList("vout");
  CHECK(vout_list);
  CHECK_LT(vout_index, vout_list->size());
  auto* out_dict = (*vout_list)[vout_index].GetIfDict();
  CHECK(out_dict);
  auto* scriptpubkey = out_dict->FindString("scriptpubkey");
  CHECK(scriptpubkey);
  CHECK(scriptpubkey->starts_with("0014"));  // OP_0 OP_PUSHBYTES_20

  std::vector<uint8_t> result;
  CHECK(base::HexStringToBytes(base::StringPiece(*scriptpubkey).substr(4),
                               &result));
  CHECK_EQ(result.size(), 20u);

  return result;
}

void PushScriptCode(SendToContext& context,
                    uint32_t input_index,
                    std::vector<uint8_t>& to) {
  to.insert(to.end(), {0x19, 0x76, 0xa9, 0x14});
  auto pubkey_hash = GetScriptPubkeyHash(context, input_index);
  to.insert(to.end(), pubkey_hash.begin(), pubkey_hash.end());
  to.insert(to.end(), {0x88, 0xac});
}

void PushHashOutputs(SendToContext& context, std::vector<uint8_t>& to) {
  std::vector<uint8_t> data;
  for (auto& output : context.outputs) {
    PushAsLE(output.amount, data);
    PushVarInt(output.script_pubkey.size(), data);
    data.insert(data.end(), output.script_pubkey.begin(),
                output.script_pubkey.end());
  }
  LOG(ERROR) << base::HexEncode(data);

  auto double_hash = DoubleHash(data);
  to.insert(to.end(), double_hash.begin(), double_hash.end());
}

bool BitcoinRpcService::FillSignature(SendToContext& context,
                                      uint32_t input_index) {
  CHECK(input_index < context.inputs.size());

  auto& input = context.inputs[input_index];

  // https://github.com/bitcoin/bips/blob/master/bip-0143.mediawiki#specification
  std::vector<uint8_t> data;
  PushAsLE(uint32_t(2), data);  // 1.
  LOG(ERROR) << base::HexEncode(data);

  PushHashPrevouts(context, data);  // 2.
  LOG(ERROR) << base::HexEncode(data);
  PushHashSequence(context, data);  // 3.
  LOG(ERROR) << base::HexEncode(data);
  PushAsLE(input.unspent_output->txid_bin, data);  // 4.
  LOG(ERROR) << base::HexEncode(data);
  PushAsLE(input.unspent_output->vout, data);  // 4.
  LOG(ERROR) << base::HexEncode(data);
  PushScriptCode(context, input_index, data);  // 5.
  LOG(ERROR) << base::HexEncode(data);
  PushAsLE(input.unspent_output->value, data);  // 6.
  LOG(ERROR) << base::HexEncode(data);
  PushAsLE(input.n_sequence, data);  // 7.
  LOG(ERROR) << base::HexEncode(data);
  PushHashOutputs(context, data);  // 8.
  LOG(ERROR) << base::HexEncode(data);
  PushAsLE(context.locktime, data);  // 9.
  LOG(ERROR) << base::HexEncode(data);
  PushAsLE(kSigHashAll, data);  // 10.
  LOG(ERROR) << base::HexEncode(data);

  auto hashed = DoubleHash(data);
  LOG(ERROR) << base::HexEncode(hashed);

  input.signature = keyring_service_->SignBitcoinMessage(
      context.keyring_id, *input.address, hashed);
  PushAsLE(uint8_t(kSigHashAll), input.signature);
  LOG(ERROR) << base::HexEncode(input.signature);
  return true;
}

bool BitcoinRpcService::FillPubkeys(SendToContext& context) {
  for (uint32_t i = 0; i < context.inputs.size(); ++i) {
    auto& input = context.inputs[i];
    input.pubkey =
        keyring_service_->GetBitcoinPubkey(context.keyring_id, *input.address);
    if (input.pubkey.empty()) {
      return false;
    }
  }

  return true;
}

bool BitcoinRpcService::FillSignatures(SendToContext& context) {
  for (uint32_t i = 0; i < context.inputs.size(); ++i) {
    if (!FillSignature(context, i)) {
      return false;
    }
  }

  return true;
}

void PushInputs(SendToContext& context, std::vector<uint8_t>& to) {
  PushVarInt(context.inputs.size(), to);
  for (auto& input : context.inputs) {
    PushAsLE(input.unspent_output->txid_bin, to);
    PushAsLE(input.unspent_output->vout, to);
    // TODO(apaymsyhev): support script for non-segwit transactions.
    PushVarInt(0, to);
    PushAsLE(input.n_sequence, to);
  }
}

void PushOutputs(SendToContext& context, std::vector<uint8_t>& to) {
  PushVarInt(context.outputs.size(), to);
  for (auto& output : context.outputs) {
    PushAsLE(output.amount, to);
    PushVarInt(output.script_pubkey.size(), to);
    to.insert(to.end(), output.script_pubkey.begin(),
              output.script_pubkey.end());
  }
}

void PushWitnesses(SendToContext& context, std::vector<uint8_t>& to) {
  for (auto& input : context.inputs) {
    // TODO(apaymyshev): only supports P2WPKH. Should support everything else.
    PushVarInt(2, to);

    PushVarSizeVector(input.signature, to);
    PushVarSizeVector(input.pubkey, to);
  }
}

bool BitcoinRpcService::FillTransaction(SendToContext& context) {
  std::vector<uint8_t> data;
  PushAsLE(uint32_t(2), data);  // version
  PushAsLE(uint8_t(0), data);   // marker
  PushAsLE(uint8_t(1), data);   // flag
  PushInputs(context, data);
  PushOutputs(context, data);
  PushWitnesses(context, data);
  PushAsLE(context.locktime, data);

  context.transaction = std::move(data);
  LOG(ERROR) << base::HexEncode(context.transaction);

  return true;
}

void BitcoinRpcService::PostTransaction(
    std::unique_ptr<SendToContext> context) {
  GURL network_url = BaseRpcUrl(context->keyring_id);
  auto payload = base::HexEncode(context->transaction);

  auto internal_callback =
      base::BindOnce(&BitcoinRpcService::OnPostTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(context));

  auto conversion_callback = base::BindOnce(&ConvertPlainStringToJsonArray);
  api_request_helper_->Request("POST", MakePostTransactionUrl(network_url),
                               payload, "", true, std::move(internal_callback),
                               {}, -1u, std::move(conversion_callback));
}

void BitcoinRpcService::OnPostTransaction(
    std::unique_ptr<SendToContext> context,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    return context->ReplyFailure();
  }

  auto* list = api_request_result.value_body().GetIfList();
  if (!list) {
    return context->ReplyFailure();
  }

  if (list->size() != 1 || !list->front().is_string()) {
    return context->ReplyFailure();
  }

  auto txid = list->front().GetString();
  std::move(context->callback)
      .Run(txid, "https://blockstream.info/testnet/tx/" + txid);
}

void BitcoinRpcService::WorkOnSendTo(std::unique_ptr<SendToContext> context) {
  DCHECK(context);

  if (context->locktime == 0) {
    auto keyring_id = context->keyring_id;

    GetChainHeight(
        keyring_id,
        base::BindOnce(&BitcoinRpcService::OnGetChainHeightForSendTo,
                       weak_ptr_factory_.GetWeakPtr(), std::move(context)));
    return;
  }

  if (!context->account_info) {
    auto keyring_id = context->keyring_id;
    auto account_index = context->account_index;
    GetBitcoinAccountInfo(
        keyring_id, account_index,
        base::BindOnce(&BitcoinRpcService::OnGetBitcoinAccountInfoForSendTo,
                       weak_ptr_factory_.GetWeakPtr(), std::move(context)));
    return;
  }

  if (!context->InputsPicked()) {
    if (!PickInputs(*context)) {
      return context->ReplyFailure();
    }
  }

  if (!context->InputTransactionsReady()) {
    FetchInputTransactions(std::move(context));
    return;
  }

  if (!context->OutputsPrepared()) {
    if (!PrepareOutputs(*context)) {
      return context->ReplyFailure();
    }
  }

  if (!context->PubkeysReady()) {
    if (!FillPubkeys(*context)) {
      return context->ReplyFailure();
    }
  }

  if (!context->SignaturesReady()) {
    if (!FillSignatures(*context)) {
      return context->ReplyFailure();
    }
  }

  if (!context->TransactionReady()) {
    if (!FillTransaction(*context)) {
      return context->ReplyFailure();
    }
  }

  PostTransaction(std::move(context));
}

// void BitcoinRpcService::ContinueSendTo(
//     std::unique_ptr<SendToContext> context,
//     mojom::BitcoinAccountInfoPtr account_info) {
//   if (!account_info) {
//     std::move(context->callback).Run("");
//     return;
//   }
// }

void BitcoinRpcService::OnGetChainHeight(GetChainHeightCallback callback,
                                         APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(0);
    return;
  }

  auto* list = api_request_result.value_body().GetIfList();
  if (!list) {
    std::move(callback).Run(0);
    return;
  }

  if (list->size() != 1 || !list->front().is_int()) {
    std::move(callback).Run(0);
    return;
  }

  std::move(callback).Run(list->front().GetInt());
}

void BitcoinRpcService::OnGetUtxoListForBitcoinAccountInfo(
    scoped_refptr<GetBitcoinAccountInfoContext> context,
    mojom::BitcoinAddressPtr requested_address,
    APIRequestResult api_request_result) {
  LOG(ERROR) << api_request_result.body();

  if (!context->callback) {
    return;
  }

  if (!api_request_result.Is2XXResponseCode()) {
    std::move(context->callback).Run(nullptr);
    return;
  }

  if (!api_request_result.value_body().is_list()) {
    std::move(context->callback).Run(nullptr);
    return;
  }

  mojom::BitcoinAddressInfo* address_info = nullptr;
  for (auto& address : context->account_info->address_infos) {
    if (address->address == requested_address) {
      address_info = address.get();
      break;
    }
  }

  if (!address_info) {
    NOTREACHED();
    std::move(context->callback).Run(nullptr);
    return;
  }

  std::vector<mojom::BitcoinUnspentOutputPtr> utxo_list;
  for (auto& item : api_request_result.value_body().GetList()) {
    auto output = BitcoinUnspentOutputFromValue(item.GetIfDict());
    if (!output) {
      std::move(context->callback).Run(nullptr);
      return;
    }

    address_info->utxo_list.push_back(std::move(output));
  }

  context->pending_addresses.erase(requested_address);

  if (context->pending_addresses.empty()) {
    for (auto& address : context->account_info->address_infos) {
      for (auto& utxo : address->utxo_list) {
        address->balance += utxo->value;
      }
      context->account_info->balance += address->balance;
    }

    std::move(context->callback).Run(std::move(context->account_info));
  }
}

void BitcoinRpcService::OnGetUtxoList(GetUtxoListCallback callback,
                                      APIRequestResult api_request_result) {
  LOG(ERROR) << api_request_result.body();
  std::move(callback).Run(api_request_result.body(), "");
}

// void BitcoinRpcService::GetTransaction(
//     const std::string& keyring_id,
//     const std::string& txid,
//     base::OnceCallback<void(base::Value)> callback) {
//   if (transactions_cache_.contains(txid)) {
//     std::move(callback).Run(transactions_cache_.at(txid));
//     return;
//   }

//   FetchTransaction(keyring_id, txid,
//   base::BindOnce(&BitcoinRpcService::OnGetTransaction,
//                      weak_ptr_factory_.GetWeakPtr(), std::move(callback))

//   GURL network_url = BaseRpcUrl(keyring_id);

//   auto internal_callback =
//       base::BindOnce(&BitcoinRpcService::OnFetchTransaction,
//                      weak_ptr_factory_.GetWeakPtr(), std::move(callback));
//   RequestInternal(true, MakeFetchTransactionUrl(network_url, txid),
//                   std::move(internal_callback));
// }

// void BitcoinRpcService::OnGetTransaction(
//     base::OnceCallback<void(base::Value)> callback,
//     base::Value transaction) {}

void BitcoinRpcService::FetchTransaction(
    const std::string& keyring_id,
    const std::string& txid,
    base::OnceCallback<void(base::Value)> callback) {
  if (transactions_cache_.contains(txid)) {
    std::move(callback).Run(transactions_cache_.at(txid).Clone());
    return;
  }

  GURL network_url = BaseRpcUrl(keyring_id);

  auto internal_callback =
      base::BindOnce(&BitcoinRpcService::OnFetchTransaction,
                     weak_ptr_factory_.GetWeakPtr(), txid, std::move(callback));
  RequestInternal(true, MakeFetchTransactionUrl(network_url, txid),
                  std::move(internal_callback));
}

void BitcoinRpcService::OnFetchTransaction(
    const std::string& txid,
    base::OnceCallback<void(base::Value)> callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(base::Value());
    return;
  }

  if (api_request_result.value_body().is_dict()) {
    transactions_cache_[txid] = api_request_result.value_body().Clone();
    std::move(callback).Run(api_request_result.value_body().Clone());
  } else {
    std::move(callback).Run(base::Value());
  }
}

void BitcoinRpcService::RequestInternal(
    bool auto_retry_on_network_change,
    const GURL& request_url,
    RequestIntermediateCallback callback,
    APIRequestHelper::ResponseConversionCallback conversion_callback) {
  DCHECK(request_url.is_valid());

  api_request_helper_->Request(
      "GET", request_url, "", "", auto_retry_on_network_change,
      std::move(callback), {}, -1u, std::move(conversion_callback));
}

}  // namespace brave_wallet
