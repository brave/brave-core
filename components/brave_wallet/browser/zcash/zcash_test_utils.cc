/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"

#include <algorithm>
#include <array>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "sql/database.h"
#include "sql/meta_table.h"
#include "sql/statement.h"

namespace brave_wallet {

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

TestingZCashWalletService::~TestingZCashWalletService() {
  sync_state_ptr = nullptr;
  sync_state().SynchronouslyResetForTest();
}

void TestingZCashWalletService::SetupSyncState(
    scoped_refptr<base::SequencedTaskRunner> sync_state_sequence,
    std::unique_ptr<OrchardSyncState> sync_state) {
  sync_state_ptr = sync_state.get();
  ZCashWalletService::SetupSyncState(std::move(sync_state_sequence),
                                     std::move(sync_state));
}

ZCashRpc& TestingZCashWalletService::zcash_rpc() {
  return ZCashWalletService::zcash_rpc();
}

OrchardSyncState::SequenceBound& TestingZCashWalletService::sync_state() {
  return ZCashWalletService::sync_state();
}

ZCashActionContext TestingZCashWalletService::CreateActionContext(
    const mojom::AccountIdPtr& account_id) {
  return ZCashWalletService::CreateActionContext(account_id);
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

OrchardNote GenerateMockOrchardNote(const mojom::AccountIdPtr& account_id,
                                    uint32_t block_id,
                                    uint8_t seed,
                                    uint64_t value) {
  return OrchardNote{
      {}, block_id, GenerateMockNullifier(account_id, seed), value, 0, {}, {}};
}

void SortByBlockId(std::vector<OrchardNote>& vec) {
  std::sort(vec.begin(), vec.end(), [](OrchardNote& a, OrchardNote& b) {
    return (a.block_id < b.block_id);
  });
}

std::vector<zcash::mojom::ZCashUtxoPtr> GetZCashUtxo(uint64_t amount) {
  auto utxo = zcash::mojom::ZCashUtxo::New();
  utxo->address = base::NumberToString(amount);
  utxo->value_zat = amount;
  utxo->tx_id = std::vector<uint8_t>(32u, 1u);
  std::vector<zcash::mojom::ZCashUtxoPtr> result;
  result.push_back(std::move(utxo));
  return result;
}

void PopulateOrchardV2DatabaseForTesting(  // IN-TEST
    const base::FilePath& db_path,
    const mojom::AccountIdPtr& account_id) {
  sql::Database database(sql::Database::Tag("PopulateOrchardV2Database"));
  CHECK(database.Open(db_path));

  sql::MetaTable meta_table;
  CHECK(meta_table.Init(&database, /*version=*/2, /*compatible_version=*/2));

  CHECK(database.Execute(
      "CREATE TABLE notes ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "account_id TEXT NOT NULL,"
      "amount INTEGER NOT NULL,"
      "addr BLOB NOT NULL,"
      "block_id INTEGER NOT NULL,"
      "commitment_tree_position INTEGER,"
      "nullifier BLOB NOT NULL UNIQUE,"
      "rho BLOB NOT NULL,"
      "rseed BLOB NOT NULL);"));

  CHECK(database.Execute(
      "CREATE TABLE spent_notes ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "account_id TEXT NOT NULL,"
      "spent_block_id INTEGER NOT NULL,"
      "nullifier BLOB NOT NULL UNIQUE);"));

  CHECK(database.Execute(
      "CREATE TABLE account_meta ("
      "account_id TEXT NOT NULL PRIMARY KEY,"
      "account_birthday INTEGER NOT NULL,"
      "latest_scanned_block INTEGER,"
      "latest_scanned_block_hash TEXT);"));

  CHECK(database.Execute(
      "CREATE TABLE shard_tree ("
      "account_id TEXT NOT NULL,"
      "shard_index INTEGER NOT NULL,"
      "subtree_end_height INTEGER,"
      "root_hash BLOB,"
      "shard_data BLOB,"
      "CONSTRAINT shard_index_unique UNIQUE (shard_index, account_id),"
      "CONSTRAINT root_unique UNIQUE (root_hash, account_id));"));

  CHECK(database.Execute(
      "CREATE TABLE checkpoints ("
      "account_id TEXT NOT NULL,"
      "checkpoint_id INTEGER NOT NULL,"
      "position INTEGER,"
      "PRIMARY KEY (checkpoint_id));"));

  CHECK(database.Execute(
      "CREATE TABLE checkpoints_mark_removed ("
      "account_id TEXT NOT NULL,"
      "checkpoint_id INTEGER NOT NULL,"
      "mark_removed_position INTEGER NOT NULL,"
      "FOREIGN KEY (checkpoint_id) REFERENCES "
      "checkpoints(checkpoint_id) ON DELETE CASCADE,"
      "CONSTRAINT spend_position_unique UNIQUE "
      "(checkpoint_id, mark_removed_position, account_id));"));

  CHECK(database.Execute(
      "CREATE TABLE shard_tree_cap ("
      "account_id TEXT NOT NULL,"
      "cap_data BLOB NOT NULL);"));

  {
    sql::Statement stmt(database.GetUniqueStatement(
        "INSERT INTO account_meta (account_id, account_birthday, "
        "latest_scanned_block, latest_scanned_block_hash) "
        "VALUES (?, ?, ?, ?)"));
    stmt.BindString(0, account_id->unique_key);
    stmt.BindInt64(1, 100);
    stmt.BindInt64(2, 105);
    stmt.BindString(3, "hash105");
    CHECK(stmt.Run());
  }

  {
    std::vector<uint8_t> addr(43, 0);
    std::vector<uint8_t> nullifier(32, 1);
    std::vector<uint8_t> rho(32, 0);
    std::vector<uint8_t> rseed(32, 0);
    sql::Statement stmt(database.GetUniqueStatement(
        "INSERT INTO notes (account_id, amount, addr, block_id, "
        "commitment_tree_position, nullifier, rho, rseed) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
    stmt.BindString(0, account_id->unique_key);
    stmt.BindInt64(1, 1000);
    stmt.BindBlob(2, addr);
    stmt.BindInt64(3, 101);
    stmt.BindInt64(4, 0);
    stmt.BindBlob(5, nullifier);
    stmt.BindBlob(6, rho);
    stmt.BindBlob(7, rseed);
    CHECK(stmt.Run());
  }

  {
    sql::Statement stmt(database.GetUniqueStatement(
        "INSERT INTO checkpoints (account_id, checkpoint_id, position) "
        "VALUES (?, ?, ?)"));
    stmt.BindString(0, account_id->unique_key);
    stmt.BindInt64(1, 1);
    stmt.BindInt64(2, 4);
    CHECK(stmt.Run());
  }
}

}  // namespace brave_wallet
