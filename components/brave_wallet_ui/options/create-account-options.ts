// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { BraveWallet, CreateAccountOptionsType } from '../constants/types'
import { getLocale } from '../../common/locale'
import { getNetworkLogo } from './asset-options'

export const CreateAccountOptions = (options: {
  visibleNetworks: BraveWallet.NetworkInfo[]
  isBitcoinEnabled: boolean
  isZCashEnabled: boolean
}): CreateAccountOptionsType[] => {
  const isNetworkVisible = (coin: BraveWallet.CoinType, chaiId: string) => {
    return !!options.visibleNetworks.find(
      (n) => n.coin === coin && n.chainId === chaiId
    )
  }

  const accounts: CreateAccountOptionsType[] = []
  const testnetAccounts: CreateAccountOptionsType[] = []

  accounts.push({
    description: getLocale('braveWalletCreateAccountEthereumDescription'),
    name: 'Ethereum',
    coin: BraveWallet.CoinType.ETH,
    icon: getNetworkLogo(BraveWallet.MAINNET_CHAIN_ID, 'ETH'),
    chainIcons: ['eth-color', 'matic-color', 'op-color', 'aurora-color']
  })

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
    fixedNetwork: BraveWallet.FILECOIN_MAINNET,
    icon: getNetworkLogo(BraveWallet.FILECOIN_MAINNET, 'FIL'),
    chainIcons: ['filecoin-color']
  })
  if (
    isNetworkVisible(BraveWallet.CoinType.FIL, BraveWallet.FILECOIN_TESTNET)
  ) {
    testnetAccounts.push({
      description: getLocale(
        'braveWalletCreateAccountFilecoinTestnetDescription'
      ),
      name: 'Filecoin Testnet',
      coin: BraveWallet.CoinType.FIL,
      fixedNetwork: BraveWallet.FILECOIN_TESTNET,
      icon: getNetworkLogo(BraveWallet.FILECOIN_TESTNET, 'FIL'),
      chainIcons: ['filecoin-color']
    })
  }

  if (options.isBitcoinEnabled) {
    accounts.push({
      description: getLocale('braveWalletCreateAccountBitcoinDescription'),
      name: 'Bitcoin',
      fixedNetwork: BraveWallet.BITCOIN_MAINNET,
      coin: BraveWallet.CoinType.BTC,
      icon: getNetworkLogo(BraveWallet.BITCOIN_MAINNET, 'BTC'),
      chainIcons: ['btc-color']
    })
    if (
      isNetworkVisible(BraveWallet.CoinType.BTC, BraveWallet.BITCOIN_TESTNET)
    ) {
      testnetAccounts.push({
        description: getLocale(
          'braveWalletCreateAccountBitcoinTestnetDescription'
        ),
        name: 'Bitcoin Testnet',
        fixedNetwork: BraveWallet.BITCOIN_TESTNET,
        coin: BraveWallet.CoinType.BTC,
        icon: getNetworkLogo(BraveWallet.BITCOIN_TESTNET, 'BTC'),
        chainIcons: ['btc-color']
      })
    }
  }

  if (options.isZCashEnabled) {
    accounts.push({
      description: getLocale('braveWalletCreateAccountZCashDescription'),
      name: 'Zcash',
      fixedNetwork: BraveWallet.Z_CASH_MAINNET,
      coin: BraveWallet.CoinType.ZEC,
      icon: getNetworkLogo(BraveWallet.Z_CASH_MAINNET, 'ZEC'),
      chainIcons: []
    })
    if (
      isNetworkVisible(BraveWallet.CoinType.ZEC, BraveWallet.Z_CASH_TESTNET)
    ) {
      testnetAccounts.push({
        description: getLocale(
          'braveWalletCreateAccountZCashTestnetDescription'
        ),
        name: 'Zcash Testnet',
        fixedNetwork: BraveWallet.Z_CASH_TESTNET,
        coin: BraveWallet.CoinType.ZEC,
        icon: getNetworkLogo(BraveWallet.Z_CASH_TESTNET, 'ZEC'),
        chainIcons: []
      })
    }
  }

  return accounts.concat(testnetAccounts)
}
