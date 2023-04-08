/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction.h"

#include <utility>

#include "base/strings/string_number_conversions.h"

namespace brave_wallet {

namespace {
bool ReadStringTo(const base::Value::Dict& dict,
                  base::StringPiece key,
                  std::string& to) {
  auto* str = dict.FindString(key);
  if (!str) {
    return false;
  }
  to = *str;
  return true;
}

bool ReadUint64To(const base::Value::Dict& dict,
                  base::StringPiece key,
                  uint64_t& to) {
  // TODO(apaymyshev): check bounds
  // TODO(apaymyshev): support reading from string
  auto i = dict.FindInt(key);
  if (!i) {
    return false;
  }
  to = *i;
  return true;
}

bool ReadUint32To(const base::Value::Dict& value,
                  base::StringPiece key,
                  uint32_t& to) {
  // TODO(apaymyshev): check bounds
  auto i = value.FindInt(key);
  if (!i) {
    return false;
  }
  to = *i;
  return true;
}

}  // namespace

/*
{
  txid: "fb61854da866beeaf7ef9e221e2170c955b76144141d1e74683146901e3d659f",
  version: 2,
  locktime: 2425275,
  vin: [
    {
      txid: "36227b84ebb57515c1882652b2ddfec95556429ef37eaf499628e3483786868c",
      vout: 0,
      prevout: {
        scriptpubkey: "00140ac37ef158a44b52a2354098d35c615a1d4023c7",
        scriptpubkey_asm:
          "OP_0 OP_PUSHBYTES_20 0ac37ef158a44b52a2354098d35c615a1d4023c7",
        scriptpubkey_type: "v0_p2wpkh",
        scriptpubkey_address: "tb1qptphau2c53949g34gzvdxhrptgw5qg78uj708c",
        value: 1000,
      },
      scriptsig: "",
      scriptsig_asm: "",
      witness: [
        "3044022052c13a7108d8eaa15ed228847ada9df014829f1ac04eaced4560644c3b6622620220723242e4e5ef653becc6b815570bbfebcf31472ca77b7c2f20a80405d67d9e8401",
        "02fda6ad637680e978a90ec98667b3b93deb3e2d030fba451f84db235bea6bb8c6",
      ],
      is_coinbase: false,
      sequence: 4294967293,
    },
    {
      txid: "68880711c3d5c7b8881a9f1df463be38e05400473de6bf53a86204ea97d9a876",
      vout: 0,
      prevout: {
        scriptpubkey: "00145eca434e156d8ebea9dcac0d3d21240577741aba",
        scriptpubkey_asm:
          "OP_0 OP_PUSHBYTES_20 5eca434e156d8ebea9dcac0d3d21240577741aba",
        scriptpubkey_type: "v0_p2wpkh",
        scriptpubkey_address: "tb1qtm9yxns4dk8ta2wu4sxn6gfyq4mhgx46s2djqf",
        value: 1000,
      },
      scriptsig: "",
      scriptsig_asm: "",
      witness: [
        "3044022040a48c50667054a4399bf9609d29147fac483dc89da1a5d8b00c65f5bf6885a202201dc45c1d3681f48671d0a8fec4ccb011e43a70d72955cc10254438ea510102f901",
        "0270f32bba12d21bceb2e93d2293b2ad4ce7f6548e61e7d8ad31c33cb556d381ac",
      ],
      is_coinbase: false,
      sequence: 4294967293,
    },
  ],
  vout: [
    {
      scriptpubkey: "00148e89dfe173318530f8535bf0dfb206a93cc82a60",
      scriptpubkey_asm:
        "OP_0 OP_PUSHBYTES_20 8e89dfe173318530f8535bf0dfb206a93cc82a60",
      scriptpubkey_type: "v0_p2wpkh",
      scriptpubkey_address: "tb1q36yalctnxxznp7znt0cdlvsx4y7vs2nquwvjw8",
      value: 1000,
    },
    {
      scriptpubkey: "00140d3ac6fd557736976aa0167a0faec426117a7eed",
      scriptpubkey_asm:
        "OP_0 OP_PUSHBYTES_20 0d3ac6fd557736976aa0167a0faec426117a7eed",
      scriptpubkey_type: "v0_p2wpkh",
      scriptpubkey_address: "tb1qp5avdl24wumfw64qzeaqltkyycgh5lhduj68ff",
      value: 700,
    },
  ],
  size: 370,
  weight: 832,
  fee: 300,
  status: {
    confirmed: true,
    block_height: 2425287,
    block_hash:
      "0000000000003be700bd4eb2662a67a9252904f43aa0151196a7053f5db4dfb9",
    block_time: 1679401889,
  },
}
*/

// static
absl::optional<Transaction> Transaction::FromRpcValue(
    const base::Value& value) {
  // TODO(apaymyshev): test this

  auto* dict = value.GetIfDict();
  if (!dict) {
    return absl::nullopt;
  }

  Transaction result;
  if (!ReadStringTo(*dict, "txid", result.txid)) {
    return absl::nullopt;
  }
  std::vector<uint8_t> txid_bin;
  if (!base::HexStringToBytes(result.txid, &txid_bin)) {
    return absl::nullopt;
  }

  auto* vin = dict->FindList("vin");
  if (!vin) {
    return absl::nullopt;
  }
  for (auto& item : *vin) {
    auto* in_dict = item.GetIfDict();
    if (!in_dict) {
      return absl::nullopt;
    }

    Input input;
    std::string txid_hex;
    if (!ReadStringTo(*in_dict, "txid", txid_hex)) {
      return absl::nullopt;
    }
    if (!base::HexStringToBytes(txid_hex, &input.outpoint.txid)) {
      return absl::nullopt;
    }

    if (!ReadUint32To(*in_dict, "vout", input.outpoint.index)) {
      return absl::nullopt;
    }

    auto* prevout_dict = in_dict->FindDict("prevout");
    if (!prevout_dict) {
      return absl::nullopt;
    }
    if (!ReadStringTo(*prevout_dict, "scriptpubkey", input.scriptpubkey)) {
      return absl::nullopt;
    }
    if (!ReadStringTo(*prevout_dict, "scriptpubkey_type",
                      input.scriptpubkey_type)) {
      return absl::nullopt;
    }
    if (!ReadStringTo(*prevout_dict, "scriptpubkey_address",
                      input.scriptpubkey_address)) {
      return absl::nullopt;
    }
    if (!ReadUint64To(*prevout_dict, "value", input.value)) {
      return absl::nullopt;
    }

    result.vin.push_back(std::move(input));
  }

  auto* vout = dict->FindList("vout");
  if (!vout) {
    return absl::nullopt;
  }
  uint32_t index = 0;
  for (auto& item : *vout) {
    auto* out_dict = item.GetIfDict();
    if (!out_dict) {
      return absl::nullopt;
    }

    Output output;
    output.outpoint.txid = txid_bin;
    output.outpoint.index = index++;

    if (!ReadUint64To(*out_dict, "value", output.value)) {
      return absl::nullopt;
    }

    if (!ReadStringTo(*out_dict, "scriptpubkey_type",
                      output.scriptpubkey_type)) {
      return absl::nullopt;
    }
    if (!ReadStringTo(*out_dict, "scriptpubkey_address",
                      output.scriptpubkey_address)) {
      return absl::nullopt;
    }

    result.vout.push_back(std::move(output));
  }

  auto* status_dict = dict->FindDict("status");
  if (!status_dict) {
    return absl::nullopt;
  }
  if (!ReadUint32To(*status_dict, "block_height", result.block_height)) {
    return absl::nullopt;
  }

  return result;
}

Outpoint::Outpoint() = default;
Outpoint::~Outpoint() = default;
Outpoint::Outpoint(const Outpoint& other) = default;
Outpoint& Outpoint::operator=(const Outpoint& other) = default;
Outpoint::Outpoint(Outpoint&& other) = default;
Outpoint& Outpoint::operator=(Outpoint&& other) = default;

Input::Input() = default;
Input::~Input() = default;
Input::Input(const Input& other) = default;
Input& Input::operator=(const Input& other) = default;
Input::Input(Input&& other) = default;
Input& Input::operator=(Input&& other) = default;

Output::Output() = default;
Output::~Output() = default;
Output::Output(const Output& other) = default;
Output& Output::operator=(const Output& other) = default;
Output::Output(Output&& other) = default;
Output& Output::operator=(Output&& other) = default;

Transaction::Transaction() = default;
Transaction::~Transaction() = default;
Transaction::Transaction(const Transaction& other) = default;
Transaction& Transaction::operator=(const Transaction& other) = default;
Transaction::Transaction(Transaction&& other) = default;
Transaction& Transaction::operator=(Transaction&& other) = default;

bool Transaction::operator<(const Transaction& other) const {
  return this->txid < other.txid;
}

}  // namespace brave_wallet
