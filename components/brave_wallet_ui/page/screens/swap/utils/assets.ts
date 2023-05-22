// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { BraveWallet } from '../../../../constants/types'
import { AccountInfoEntity } from '../../../../common/slices/entities/account-info.entity'

export const getBalanceRegistryKey = (
  account: AccountInfoEntity,
  asset: BraveWallet.BlockchainToken
) => {
  return `${account.address.toLocaleLowerCase() //
    }-${asset.coin //
    }-${asset.chainId //
    }-${asset.contractAddress.toLowerCase()}`
}

export const getBalanceRegistryKeyRaw = (
  account: AccountInfoEntity,
  coin: BraveWallet.CoinType,
  chainId: string,
  contract: string
) => {
  return `${account.address.toLocaleLowerCase() //
    }-${coin //
    }-${chainId //
    }-${contract.toLowerCase()}`
}
