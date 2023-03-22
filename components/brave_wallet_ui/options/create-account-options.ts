// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { BraveWallet, CreateAccountOptionsType } from '../constants/types'
import { getLocale } from '../../common/locale'
import { getNetworkLogo } from './asset-options'

export const CreateAccountOptions = (isFilecoinEnabled: boolean, isSolanaEnabled: boolean): CreateAccountOptionsType[] => {
  let accounts = [
    {
      description: getLocale('braveWalletCreateAccountEthereumDescription'),
      name: 'Ethereum',
      coin: BraveWallet.CoinType.ETH,
      icon: getNetworkLogo(BraveWallet.MAINNET_CHAIN_ID, 'ETH')
    }
  ]
  if (isSolanaEnabled) {
    accounts.push({
      description: getLocale('braveWalletCreateAccountSolanaDescription'),
      name: 'Solana',
      coin: BraveWallet.CoinType.SOL,
      icon: getNetworkLogo(BraveWallet.SOLANA_MAINNET, 'SOL')
    })
  }
  if (isFilecoinEnabled) {
    accounts.push({
      description: getLocale('braveWalletCreateAccountFilecoinDescription'),
      name: 'Filecoin',
      coin: BraveWallet.CoinType.FIL,
      icon: getNetworkLogo(BraveWallet.FILECOIN_MAINNET, 'FIL')
    })
  }
  return accounts
}
