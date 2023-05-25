// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { AccountInfoEntity } from '../../../../common/slices/entities/account-info.entity'

export const getBalanceRegistryKey = (
  account: AccountInfoEntity,
  chainId: string,
  contract: string
) => {
  return `${account.address.toLocaleLowerCase() //
    }-${account.coin //
    }-${chainId //
    }-${contract.toLowerCase()}`
}
