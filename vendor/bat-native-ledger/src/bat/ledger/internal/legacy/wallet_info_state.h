/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_LEGACY_WALLET_INFO_STATE_H_
#define BRAVELEDGER_LEGACY_WALLET_INFO_STATE_H_

#include <string>

#include "bat/ledger/internal/legacy/state_reader.h"
#include "bat/ledger/internal/legacy/state_writer.h"
#include "bat/ledger/internal/legacy/wallet_info_properties.h"
#include "rapidjson/writer.h"

namespace ledger {

using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

class WalletInfoState
    : public state::Reader<WalletInfoProperties>,
      public state::Writer<JsonWriter*, WalletInfoProperties> {
 public:
  WalletInfoState();
  ~WalletInfoState();

  bool FromJson(
      const std::string& json,
      WalletInfoProperties* properties) const override;

  bool FromDict(
      const base::DictionaryValue* dictionary,
      WalletInfoProperties* properties) const override;

  bool ToJson(
      JsonWriter* writer,
      const WalletInfoProperties& properties) const override;

  std::string ToJson(
      const WalletInfoProperties& properties) const override;
};

}  // namespace ledger

#endif  // BRAVELEDGER_LEGACY_WALLET_INFO_STATE_H_
