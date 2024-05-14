// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { BraveWallet, CreateAccountOptionsType } from '../constants/types'
import { getLocale } from '../../common/locale'
import { getNetworkLogo } from './asset-options'

export const CreateAccountOptions = (options: {
  isBitcoinEnabled: boolean
  isZCashEnabled: boolean
}): CreateAccountOptionsType[] => {
  let accounts = [
    {
      description: getLocale('braveWalletCreateAccountEthereumDescription'),
      name: 'Ethereum',
      coin: BraveWallet.CoinType.ETH,
      icon: getNetworkLogo(BraveWallet.MAINNET_CHAIN_ID, 'ETH'),
      chainIcons: ['eth-color', 'matic-color', 'op-color', 'aurora-color']
    }
  ]
  accounts.push({
    description: getLocale('braveWalletCreateAccountSolanaDescription'),
    name: 'Solana',
    coin: BraveWallet.CoinType.SOL,
    icon: getNetworkLogo(BraveWallet.SOLANA_MAINNET, 'SOL'),
    chainIcons: ['sol-color']
  })

  accounts.push({
    description: getLocale('braveWalletCreateAccountFilecoinDescription'),
    name: 'Filecoin',
    coin: BraveWallet.CoinType.FIL,
    icon: getNetworkLogo(BraveWallet.FILECOIN_MAINNET, 'FIL'),
    chainIcons: ['filecoin-color']
  })

  if (options.isBitcoinEnabled) {
    accounts.push({
      description: getLocale('braveWalletCreateAccountBitcoinDescription'),
      name: 'Bitcoin',
      coin: BraveWallet.CoinType.BTC,
      icon: getNetworkLogo(BraveWallet.BITCOIN_MAINNET, 'BTC'),
      chainIcons: []
    })
  }
  if (options.isZCashEnabled) {
    accounts.push({
      description: getLocale('braveWalletCreateAccountZCashDescription'),
      name: 'ZCash',
      coin: BraveWallet.CoinType.ZEC,
      icon: getNetworkLogo(BraveWallet.Z_CASH_MAINNET, 'ZEC'),
      chainIcons: []
    })
  }
  return accounts
}
