// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {
  BraveWallet,
  WalletAccountType
} from '../../constants/types'

export const getMockedTransactionInfo = (): BraveWallet.TransactionInfo => {
  return {
    id: '1',
    fromAddress: '0x8b52c24d6e2600bdb8dbb6e8da849ed38ab7e81f',
    txHash: '',
    txData: {
      baseData: {
        to: '0x8b52c24d6e2600bdb8dbb6e8da849ed38ab7e81f',
        value: '0x01706a99bf354000',
        data: new Uint8Array(0),
        nonce: '0x03',
        gasLimit: '0x5208',
        gasPrice: '0x22ecb25c00'
      },
      chainId: '1337',
      maxPriorityFeePerGas: '',
      maxFeePerGas: ''
    },
    txStatus: BraveWallet.TransactionStatus.Approved,
    txType: BraveWallet.TransactionType.Other,
    txParams: [],
    txArgs: [],
    createdTime: { microseconds: 0 },
    submittedTime: { microseconds: 0 },
    confirmedTime: { microseconds: 0 }
  }
}

export const mockNetwork: BraveWallet.EthereumChain = {
  chainId: '0x1',
  chainName: 'Ethereum Main Net',
  rpcUrls: ['https://mainnet.infura.io/v3/'],
  blockExplorerUrls: ['https://etherscan.io/'],
  symbol: 'ETH',
  symbolName: 'Ethereum',
  decimals: 18,
  iconUrls: [],
  isEip1559: true
}

export const mockERC20Token: BraveWallet.BlockchainToken = {
  contractAddress: 'mockContractAddress',
  name: 'Dog Coin',
  symbol: 'DOG',
  logo: '',
  isErc20: true,
  isErc721: false,
  decimals: 18,
  visible: true,
  tokenId: '',
  coingeckoId: ''
}

export const mockAccount: WalletAccountType = {
  id: 'mockId',
  name: 'mockAccountName',
  address: 'mockAddress',
  balance: '123456',
  accountType: 'Primary',
  tokenBalanceRegistry: {}
}

export const mockAssetPrices: BraveWallet.AssetPrice[] = [
  {
    fromAsset: 'ETH',
    price: '4000',
    toAsset: 'mockValue',
    assetTimeframeChange: 'mockValue'
  },
  {
    fromAsset: 'DOG',
    price: '100',
    toAsset: 'mockValue',
    assetTimeframeChange: 'mockValue'
  }
]
