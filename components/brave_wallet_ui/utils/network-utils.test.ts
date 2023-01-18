// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import {
  getNetworkInfo,
  emptyNetwork,
  getNetworksByCoinType,
  getTokensNetwork,
  getCoinFromTxDataUnion,
  getFilecoinKeyringIdFromNetwork
} from './network-utils'
import { mockNetworks } from '../stories/mock-data/mock-networks'
import { BraveWallet } from '../constants/types'
import { mockNewAssetOptions } from '../stories/mock-data/mock-asset-options'

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

describe('getNetworksByCoinType', () => {
  it('CoinType ETH, should return all ETH networks', () => {
    expect(
      getNetworksByCoinType(mockNetworks, BraveWallet.CoinType.ETH)
    ).toEqual(mockNetworks.filter((n) => n.coin === BraveWallet.CoinType.ETH))
  })
  it('CoinType random number, should return an empty array', () => {
    expect(getNetworksByCoinType(mockNetworks, 3000)).toEqual([])
  })
})

describe('getTokensNetwork', () => {
  it('Ethereum with chainId 0x1, should return ETH Mainnet info', () => {
    expect(getTokensNetwork(mockNetworks, ethToken)).toEqual(ethMainNetwork)
  })
  it('Binance Coin with chainId 0x5, should return ETH Goerli Testnetwork info', () => {
    expect(getTokensNetwork(mockNetworks, bnbToken)).toEqual(mockNetworks[1])
  })
})

describe('getCoinFromTxDataUnion', () => {
  it('Filecoin transaction', () => {
    expect(getCoinFromTxDataUnion({
      filTxData: {} as BraveWallet.FilTxData,
      ethTxData: undefined,
      ethTxData1559: undefined,
      solanaTxData: undefined
    })).toEqual(BraveWallet.CoinType.FIL)
  })
  it('Ethereum transaction', () => {
    expect(getCoinFromTxDataUnion({
      filTxData: undefined,
      ethTxData: {} as BraveWallet.TxData,
      ethTxData1559: undefined,
      solanaTxData: undefined
    })).toEqual(BraveWallet.CoinType.ETH)
  })
  it('Ethereum1559 transaction', () => {
    expect(getCoinFromTxDataUnion({
      filTxData: undefined,
      ethTxData: undefined,
      ethTxData1559: {} as BraveWallet.TxData1559,
      solanaTxData: undefined
    })).toEqual(BraveWallet.CoinType.ETH)
  })
  it('Solana transaction', () => {
    expect(getCoinFromTxDataUnion({
      filTxData: undefined,
      ethTxData: undefined,
      ethTxData1559: undefined,
      solanaTxData: {} as BraveWallet.SolanaTxData
    })).toEqual(BraveWallet.CoinType.SOL)
  })
})

describe('getFilecoinKeyringIdFromNetwork', () => {
  it('Filecoin testnet', () => {
    expect(getFilecoinKeyringIdFromNetwork({ chainId: BraveWallet.FILECOIN_TESTNET, coin: BraveWallet.CoinType.FIL } as BraveWallet.NetworkInfo)).toEqual(BraveWallet.FILECOIN_TESTNET_KEYRING_ID)
  })
  it('Filecoin localhost', () => {
    expect(getFilecoinKeyringIdFromNetwork({ chainId: BraveWallet.LOCALHOST_CHAIN_ID, coin: BraveWallet.CoinType.FIL } as BraveWallet.NetworkInfo)).toEqual(BraveWallet.FILECOIN_TESTNET_KEYRING_ID)
  })
  it('Filecoin mainnet', () => {
    expect(getFilecoinKeyringIdFromNetwork({ chainId: BraveWallet.FILECOIN_MAINNET, coin: BraveWallet.CoinType.FIL } as BraveWallet.NetworkInfo)).toEqual(BraveWallet.FILECOIN_KEYRING_ID)
  })
  it('Non filecoin', () => {
    expect(getFilecoinKeyringIdFromNetwork({ chainId: BraveWallet.GOERLI_CHAIN_ID, coin: BraveWallet.CoinType.ETH } as BraveWallet.NetworkInfo)).toEqual(undefined)
  })
})
