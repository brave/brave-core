/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TEST_UTILS_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_wallet/browser/account_resolver_delegate.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/value_store/test_value_store_factory.h"
#include "components/value_store/value_store_frontend.h"

class PrefService;

namespace base {
class ScopedTempDir;
}  // namespace base

namespace brave_wallet {

constexpr char kMnemonicDivideCruise[] =
    "divide cruise upon flag harsh carbon filter merit once advice bright "
    "drive";

class AccountResolverDelegateForTest;
class TxStorageDelegate;
class TxStorageDelegateImpl;

class AccountResolverDelegateForTest : public AccountResolverDelegate {
 public:
  AccountResolverDelegateForTest();
  ~AccountResolverDelegateForTest() override;

  mojom::AccountIdPtr RegisterAccount(mojom::AccountIdPtr account_id);

 protected:
  mojom::AccountIdPtr ResolveAccountId(
      const std::string* from_account_id,
      const std::string* from_address) override;

 private:
  std::vector<mojom::AccountIdPtr> accounts_;
};

void WaitForTxStorageDelegateInitialized(TxStorageDelegate* delegate);

scoped_refptr<value_store::TestValueStoreFactory> GetTestValueStoreFactory(
    base::ScopedTempDir& temp_dir);

std::unique_ptr<TxStorageDelegateImpl> GetTxStorageDelegateForTest(
    PrefService* prefs,
    scoped_refptr<value_store::ValueStoreFactory> store_factory);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TEST_UTILS_H_
