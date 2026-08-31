// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet, CreateAccountOptionsType } from '../constants/types'
import { getLocale } from '../../common/locale'
import { getCreateAccountLogo } from './asset-options'

export const CreateAccountOptions = (options: {
  visibleNetworks: BraveWallet.NetworkInfo[]
  isBitcoinEnabled: boolean
  isZCashEnabled: boolean
  isCardanoEnabled: boolean
  isPolkadotEnabled: boolean
}): CreateAccountOptionsType[] => {
  const isNetworkVisible = (coin: BraveWallet.CoinType, chaiId: string) => {
    return !!options.visibleNetworks.find(
      (n) => n.coin === coin && n.chainId === chaiId,
    )
  }

  const accounts: CreateAccountOptionsType[] = []
  const testnetAccounts: CreateAccountOptionsType[] = []

  accounts.push({
    description: getLocale(S.BRAVE_WALLET_CREATE_ACCOUNT_ETHEREUM_DESCRIPTION),
    name: 'Ethereum',
    coin: BraveWallet.CoinType.ETH,
    icon: getCreateAccountLogo(BraveWallet.CoinType.ETH),
    chainIcons: ['eth-color', 'matic-color', 'op-color', 'aurora-color'],
  })

  accounts.push({
    description: getLocale(S.BRAVE_WALLET_CREATE_ACCOUNT_SOLANA_DESCRIPTION),
    name: 'Solana',
    coin: BraveWallet.CoinType.SOL,
    icon: getCreateAccountLogo(BraveWallet.CoinType.SOL),
    chainIcons: ['sol-color'],
  })

  accounts.push({
    description: getLocale(S.BRAVE_WALLET_CREATE_ACCOUNT_FILECOIN_DESCRIPTION),
    name: 'Filecoin',
    coin: BraveWallet.CoinType.FIL,
    fixedNetwork: BraveWallet.FILECOIN_MAINNET,
    icon: getCreateAccountLogo(BraveWallet.CoinType.FIL),
    chainIcons: ['filecoin-color'],
  })
  if (
    isNetworkVisible(BraveWallet.CoinType.FIL, BraveWallet.FILECOIN_TESTNET)
  ) {
    testnetAccounts.push({
      description: getLocale(
        S.BRAVE_WALLET_CREATE_ACCOUNT_FILECOIN_TESTNET_DESCRIPTION,
      ),
      name: 'Filecoin Testnet',
      coin: BraveWallet.CoinType.FIL,
      fixedNetwork: BraveWallet.FILECOIN_TESTNET,
      icon: getCreateAccountLogo(BraveWallet.CoinType.FIL),
      chainIcons: ['filecoin-color'],
    })
  }

  if (options.isBitcoinEnabled) {
    accounts.push({
      description: getLocale(S.BRAVE_WALLET_CREATE_ACCOUNT_BITCOIN_DESCRIPTION),
      name: 'Bitcoin',
      fixedNetwork: BraveWallet.BITCOIN_MAINNET,
      coin: BraveWallet.CoinType.BTC,
      icon: getCreateAccountLogo(BraveWallet.CoinType.BTC),
      chainIcons: ['btc-color'],
    })
    if (
      isNetworkVisible(BraveWallet.CoinType.BTC, BraveWallet.BITCOIN_TESTNET)
    ) {
      testnetAccounts.push({
        description: getLocale(
          S.BRAVE_WALLET_CREATE_ACCOUNT_BITCOIN_TESTNET_DESCRIPTION,
        ),
        name: 'Bitcoin Testnet',
        fixedNetwork: BraveWallet.BITCOIN_TESTNET,
        coin: BraveWallet.CoinType.BTC,
        icon: getCreateAccountLogo(BraveWallet.CoinType.BTC),
        chainIcons: ['btc-color'],
      })
    }
  }

  if (options.isZCashEnabled) {
    accounts.push({
      description: getLocale(S.BRAVE_WALLET_CREATE_ACCOUNT_ZCASH_DESCRIPTION),
      name: 'Zcash',
      fixedNetwork: BraveWallet.Z_CASH_MAINNET,
      coin: BraveWallet.CoinType.ZEC,
      icon: getCreateAccountLogo(BraveWallet.CoinType.ZEC),
      chainIcons: [],
    })
    if (
      isNetworkVisible(BraveWallet.CoinType.ZEC, BraveWallet.Z_CASH_TESTNET)
    ) {
      testnetAccounts.push({
        description: getLocale(
          S.BRAVE_WALLET_CREATE_ACCOUNT_ZCASH_TESTNET_DESCRIPTION,
        ),
        name: 'Zcash Testnet',
        fixedNetwork: BraveWallet.Z_CASH_TESTNET,
        coin: BraveWallet.CoinType.ZEC,
        icon: getCreateAccountLogo(BraveWallet.CoinType.ZEC),
        chainIcons: [],
      })
    }
  }

  if (options.isCardanoEnabled) {
    accounts.push({
      description: getLocale(S.BRAVE_WALLET_CREATE_ACCOUNT_CARDANO_DESCRIPTION),
      name: 'Cardano',
      fixedNetwork: BraveWallet.CARDANO_MAINNET,
      coin: BraveWallet.CoinType.ADA,
      icon: getCreateAccountLogo(BraveWallet.CoinType.ADA),
      chainIcons: ['ada-color'],
    })
    if (
      isNetworkVisible(BraveWallet.CoinType.ADA, BraveWallet.CARDANO_TESTNET)
    ) {
      testnetAccounts.push({
        description: getLocale(
          S.BRAVE_WALLET_CREATE_ACCOUNT_CARDANO_TESTNET_DESCRIPTION,
        ),
        name: 'Cardano Testnet',
        fixedNetwork: BraveWallet.CARDANO_TESTNET,
        coin: BraveWallet.CoinType.ADA,
        icon: getCreateAccountLogo(BraveWallet.CoinType.ADA),
        chainIcons: ['ada-color'],
      })
    }
  }

  if (options.isPolkadotEnabled) {
    accounts.push({
      description: getLocale(
        S.BRAVE_WALLET_CREATE_ACCOUNT_POLKADOT_DESCRIPTION,
      ),
      name: 'Polkadot',
      coin: BraveWallet.CoinType.DOT,
      fixedNetwork: BraveWallet.POLKADOT_MAINNET,
      icon: getCreateAccountLogo(BraveWallet.CoinType.DOT),
      chainIcons: ['dot-color'],
    })

    if (
      isNetworkVisible(BraveWallet.CoinType.DOT, BraveWallet.POLKADOT_TESTNET)
    ) {
      testnetAccounts.push({
        description: getLocale(
          S.BRAVE_WALLET_CREATE_ACCOUNT_POLKADOT_TESTNET_DESCRIPTION,
        ),
        name: 'Polkadot Westend',
        fixedNetwork: BraveWallet.POLKADOT_TESTNET,
        coin: BraveWallet.CoinType.DOT,
        icon: getCreateAccountLogo(BraveWallet.CoinType.DOT),
        chainIcons: ['dot-color'],
      })
    }
  }

  return accounts.concat(testnetAccounts)
}
