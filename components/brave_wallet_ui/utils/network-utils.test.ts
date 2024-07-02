// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import {
  getNetworkInfo,
  emptyNetwork,
  filterNetworksForAccount,
  getTokensNetwork,
  getCoinFromTxDataUnion,
  networkSupportsAccount
} from './network-utils'
import {
  mockBitcoinMainnet,
  mockBitcoinTestnet,
  mockEthMainnet,
  mockNetworks
} from '../stories/mock-data/mock-networks'
import { BraveWallet } from '../constants/types'
import { mockNewAssetOptions } from '../stories/mock-data/mock-asset-options'
import {
  mockBitcoinAccount,
  mockEthAccount
} from '../stories/mock-data/mock-wallet-accounts'

const ethToken = mockNewAssetOptions[0]
const bnbToken = mockNewAssetOptions[2]
const ethMainNetwork = mockNetworks[0]

describe('getNetworkInfo', () => {
  it('should return network info', () => {
    const { chainId, coin } = ethMainNetwork
    expect(getNetworkInfo(chainId, coin, mockNetworks)).toEqual(ethMainNetwork)
  })

  it('should return network object with default values if network with chainId is not found', () => {
    const chainId = 'fakeChainId'
    const coin = BraveWallet.CoinType.ETH
    expect(getNetworkInfo(chainId, coin, mockNetworks)).toEqual(emptyNetwork)
  })
})

describe('networkSupportsAccount', () => {
  it('ETH mainnet should match ETH account', () => {
    expect(
      networkSupportsAccount(mockEthMainnet, mockEthAccount.accountId)
    ).toBeTruthy()
  })
  it('ETH mainnet should not match BTC account', () => {
    expect(
      networkSupportsAccount(mockEthMainnet, mockBitcoinAccount.accountId)
    ).toBeFalsy()
  })
  it('BTC mainnet should match Bitcoin mainnet account', () => {
    expect(
      networkSupportsAccount(mockBitcoinMainnet, mockBitcoinAccount.accountId)
    ).toBeTruthy()
  })
  it('BTC testnet should not match Bitcoin mainnet account', () => {
    expect(
      networkSupportsAccount(mockBitcoinTestnet, mockBitcoinAccount.accountId)
    ).toBeFalsy()
  })
})

describe('filterNetworksForAccount', () => {
  it('CoinType ETH, should return all ETH networks', () => {
    expect(
      filterNetworksForAccount(mockNetworks, mockEthAccount.accountId)
    ).toEqual(mockNetworks.filter((n) => n.coin === BraveWallet.CoinType.ETH))
  })
  it('CoinType BTC, should return Bitcoin mainnet', () => {
    expect(
      filterNetworksForAccount(mockNetworks, mockBitcoinAccount.accountId)
    ).toEqual([mockBitcoinMainnet])
  })
})

describe('getTokensNetwork', () => {
  it('Ethereum with chainId 0x1, should return ETH Mainnet info', () => {
    expect(getTokensNetwork(mockNetworks, ethToken)).toEqual(ethMainNetwork)
  })
  it('Binance Coin with chainId 0xaa36a7, should return ETH Sepolia Testnetwork info', () => {
    expect(getTokensNetwork(mockNetworks, bnbToken)).toEqual(mockNetworks[1])
  })
})

describe('getCoinFromTxDataUnion', () => {
  it('Filecoin transaction', () => {
    expect(
      getCoinFromTxDataUnion({
        filTxData: {} as BraveWallet.FilTxData,
        ethTxData: undefined,
        ethTxData1559: undefined,
        solanaTxData: undefined,
        btcTxData: undefined
      })
    ).toEqual(BraveWallet.CoinType.FIL)
  })
  it('Ethereum transaction', () => {
    expect(
      getCoinFromTxDataUnion({
        filTxData: undefined,
        ethTxData: {} as BraveWallet.TxData,
        ethTxData1559: undefined,
        solanaTxData: undefined,
        btcTxData: undefined
      })
    ).toEqual(BraveWallet.CoinType.ETH)
  })
  it('Ethereum1559 transaction', () => {
    expect(
      getCoinFromTxDataUnion({
        filTxData: undefined,
        ethTxData: undefined,
        ethTxData1559: {} as BraveWallet.TxData1559,
        solanaTxData: undefined,
        btcTxData: undefined
      })
    ).toEqual(BraveWallet.CoinType.ETH)
  })
  it('Solana transaction', () => {
    expect(
      getCoinFromTxDataUnion({
        filTxData: undefined,
        ethTxData: undefined,
        ethTxData1559: undefined,
        solanaTxData: {} as BraveWallet.SolanaTxData,
        btcTxData: undefined
      })
    ).toEqual(BraveWallet.CoinType.SOL)
  })
  it('Bitcoin transaction', () => {
    expect(
      getCoinFromTxDataUnion({
        filTxData: undefined,
        ethTxData: undefined,
        ethTxData1559: undefined,
        solanaTxData: undefined,
        btcTxData: {} as BraveWallet.BtcTxData
      })
    ).toEqual(BraveWallet.CoinType.BTC)
  })
})
