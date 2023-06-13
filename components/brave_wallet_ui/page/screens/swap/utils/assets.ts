// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { BraveWallet } from '../../../../constants/types'

export const getBalanceRegistryKey = (
  accountId: BraveWallet.AccountId,
  chainId: string,
  contract: string
) => {
  // TODO(apaymyshev): is it persisted? can we use accountId.uniqueKey?
  return `${accountId.address.toLocaleLowerCase() //
    }-${accountId.coin //
    }-${chainId //
    }-${contract.toLowerCase()}`
}
