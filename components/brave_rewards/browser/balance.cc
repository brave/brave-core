/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/balance.h"

#include <memory>
#include <utility>

#include "base/values.h"
#include "base/json/json_writer.h"

namespace brave_rewards {

Balance::Balance() : total(0.0) {}

Balance::~Balance() {}

Balance::Balance(const Balance& properties) {
  total = properties.total;
  wallets = properties.wallets;
}

std::string Balance::toJson() {
  std::string json_str_root;
  base::DictionaryValue json_root;
  json_root.SetDoubleKey(kJsonTotal, total);

  auto json_wallets = std::make_unique<base::DictionaryValue>();
  for (const auto & item : wallets) {
    json_wallets->SetDoubleKey(item.first, item.second);
  }
  json_root.SetDictionary(kJsonWallets, std::move(json_wallets));

  base::JSONWriter::Write(json_root, &json_str_root);
  return json_str_root;
}

}  // namespace brave_rewards
