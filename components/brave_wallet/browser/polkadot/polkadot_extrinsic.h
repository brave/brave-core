/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_EXTRINSIC_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_EXTRINSIC_H_

#include <stdint.h>

#include <array>
#include <string>
#include <string_view>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

class PolkadotExtrinsicMetadata {
 public:
  PolkadotExtrinsicMetadata();
  ~PolkadotExtrinsicMetadata();

  PolkadotExtrinsicMetadata(const PolkadotExtrinsicMetadata&);
  PolkadotExtrinsicMetadata& operator=(const PolkadotExtrinsicMetadata&);

  PolkadotExtrinsicMetadata(PolkadotExtrinsicMetadata&&);
  PolkadotExtrinsicMetadata& operator=(PolkadotExtrinsicMetadata&&);

  base::DictValue ToValue() const;
  static std::optional<PolkadotExtrinsicMetadata> FromValue(
      const base::DictValue& value);

  auto block_hash() const { return block_hash_; }
  void set_block_hash(std::array<uint8_t, kPolkadotBlockHashSize> block_hash) {
    block_hash_ = block_hash;
  }

  const std::vector<uint8_t>& extrinsic() const { return extrinsic_; }
  void set_extrinsic(std::vector<uint8_t> extrinsic) {
    extrinsic_ = std::move(extrinsic);
  }

  uint32_t block_num() const { return block_num_; }
  void set_block_num(uint32_t block_num) { block_num_ = block_num; }

  uint32_t mortality_period() const { return mortality_period_; }
  void set_mortality_period(uint32_t mortality_period) {
    mortality_period_ = mortality_period;
  }

 private:
  std::array<uint8_t, kPolkadotBlockHashSize> block_hash_ = {};
  std::vector<uint8_t> extrinsic_;
  uint32_t block_num_ = 0;

  // Right now we don't necessarily need to store the mortality period used for
  // the extrinsic. But in the future, we could wind up with extrinsics with
  // individual mortality periods so it's best to build the scaffolding for this
  // now.
  uint32_t mortality_period_ = 64;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_EXTRINSIC_H_
