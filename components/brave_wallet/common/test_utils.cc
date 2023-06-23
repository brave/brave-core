/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/test_utils.h"

#include "base/files/scoped_temp_dir.h"
#include "base/task/sequenced_task_runner.h"
#include "components/value_store/value_store_task_runner.h"

namespace brave_wallet {

mojom::NetworkInfo GetTestNetworkInfo1(const std::string& chain_id) {
  return {chain_id,
          "chain_name",
          {"https://url1.com"},
          {"https://url1.com"},
          0,
          {GURL("https://url1.com")},
          "symbol",
          "symbol_name",
          11,
          mojom::CoinType::ETH,
          false};
}

mojom::NetworkInfo GetTestNetworkInfo2(const std::string& chain_id) {
  return {chain_id,
          "chain_name2",
          {"https://url2.com"},
          {"https://url2.com"},
          0,
          {GURL("https://url2.com")},
          "symbol2",
          "symbol_name2",
          22,
          mojom::CoinType::ETH,
          true};
}

scoped_refptr<value_store::TestValueStoreFactory> GetTestValueStoreFactory(
    base::ScopedTempDir& temp_dir) {
  CHECK(temp_dir.CreateUniqueTempDir());

  base::FilePath db_path = temp_dir.GetPath().AppendASCII("temp_db");

  return new value_store::TestValueStoreFactory(db_path);
}

std::unique_ptr<value_store::ValueStoreFrontend> GetValueStoreFrontendForTest(
    const scoped_refptr<value_store::ValueStoreFactory>& store_factory) {
  return std::make_unique<value_store::ValueStoreFrontend>(
      store_factory,
      base::FilePath(FILE_PATH_LITERAL("Brave Wallet Storage Test")),
      "BraveWallet", base::SequencedTaskRunner::GetCurrentDefault(),
      value_store::GetValueStoreTaskRunner());
}

}  // namespace brave_wallet
