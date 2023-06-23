/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_TEST_UTILS_H_

#include <memory>
#include <string>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/value_store/test_value_store_factory.h"
#include "components/value_store/value_store_frontend.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"

namespace base {
class ScopedTempDir;
}  // namespace base

namespace brave_wallet {

mojom::NetworkInfo GetTestNetworkInfo1(
    const std::string& chain_id = "chain_id");
mojom::NetworkInfo GetTestNetworkInfo2(
    const std::string& chain_id = "chain_id2");

// Matcher to check equality of two mojo structs. Matcher needs copyable value
// which is not possible for some mojo types, so wrapping it with RefCounted.
template <typename T>
auto EqualsMojo(const T& value) {
  return testing::Truly(
      [value = base::MakeRefCounted<base::RefCountedData<T>>(value.Clone())](
          const T& candidate) { return mojo::Equals(candidate, value->data); });
}

scoped_refptr<value_store::TestValueStoreFactory> GetTestValueStoreFactory(
    base::ScopedTempDir& temp_dir);

std::unique_ptr<value_store::ValueStoreFrontend> GetValueStoreFrontendForTest(
    const scoped_refptr<value_store::ValueStoreFactory>& store_factory);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_TEST_UTILS_H_
