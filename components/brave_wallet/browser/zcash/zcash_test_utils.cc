/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace brave_wallet {

// MockOrchardBlockScannerProxy
MockOrchardBlockScannerProxy::MockOrchardBlockScannerProxy(Callback callback)
    : OrchardBlockScannerProxy({}), callback_(callback) {}

MockOrchardBlockScannerProxy::~MockOrchardBlockScannerProxy() = default;

void MockOrchardBlockScannerProxy::ScanBlocks(
    OrchardTreeState tree_state,
    std::vector<zcash::mojom::CompactBlockPtr> blocks,
    base::OnceCallback<void(base::expected<OrchardBlockScanner::Result,
                                           OrchardBlockScanner::ErrorCode>)>
        callback) {
  callback_.Run(std::move(tree_state), std::move(blocks), std::move(callback));
}

OrchardNullifier GenerateMockNullifier(const mojom::AccountIdPtr& account_id,
                                       uint8_t seed) {
  std::array<uint8_t, kOrchardNullifierSize> nullifier;
  nullifier.fill(seed);
  nullifier[0] = account_id->account_index;
  return nullifier;
}

OrchardNoteSpend GenerateMockNoteSpend(const mojom::AccountIdPtr& account_id,
                                       uint32_t block_id,
                                       uint8_t seed) {
  return OrchardNoteSpend{block_id, GenerateMockNullifier(account_id, seed)};
}

OrchardNote GenerateMockOrchardNote(const mojom::AccountIdPtr& account_id,
                                    uint32_t block_id,
                                    uint8_t seed) {
  return OrchardNote{{},
                     block_id,
                     GenerateMockNullifier(account_id, seed),
                     static_cast<uint32_t>(seed * 10),
                     0,
                     {},
                     {}};
}

void SortByBlockId(std::vector<OrchardNote>& vec) {
  std::sort(vec.begin(), vec.end(), [](OrchardNote& a, OrchardNote& b) {
    return (a.block_id < b.block_id);
  });
}

std::vector<zcash::mojom::ZCashUtxoPtr> GetZCashUtxo(size_t seed) {
  auto utxo = zcash::mojom::ZCashUtxo::New();
  utxo->address = base::NumberToString(seed);
  utxo->value_zat = seed;
  utxo->tx_id = std::vector<uint8_t>(32u, 1u);
  std::vector<zcash::mojom::ZCashUtxoPtr> result;
  result.push_back(std::move(utxo));
  return result;
}

}  // namespace brave_wallet
