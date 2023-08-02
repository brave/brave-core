// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Provider } from 'react-redux'
import { renderHook } from '@testing-library/react-hooks'

import { useGetTransactionsQuery } from './api.slice'

import {
  mockEthAccountInfo,
  mockFilecoinAccountInfo,
  mockSolanaAccountInfo
} from '../constants/mocks'
import { createMockStore } from '../../utils/test-utils'
import {
  createMockTransactionInfo
} from '../../stories/mock-data/mock-transaction-info'
import { BraveWallet, CoinType } from '../../constants/types'
import { mockAccounts } from '../../stories/mock-data/mock-wallet-accounts'
import {
  mockBasicAttentionToken,
  mockBitcoinErc20Token
} from '../../stories/mock-data/mock-asset-options'

function renderHookOptionsWithCustomStore (store: any) {
  return {
    wrapper: ({ children }: { children?: React.ReactChildren }) =>
      <Provider store={store}>
          {children}
      </Provider>
  }
}

const mockSolanaSendTokenTx = createMockTransactionInfo({
  chainId: BraveWallet.SOLANA_MAINNET,
  coinType: CoinType.SOL,
  fromAddress: mockSolanaAccountInfo.address,
  toAddress: 'sSolanaAccount2',
  sendApproveOrSellAssetContractAddress:
    mockBasicAttentionToken.contractAddress,
  sendApproveOrSellAmount: '100'
})

const mockFilSendTx = createMockTransactionInfo({
  chainId: BraveWallet.FILECOIN_MAINNET,
  coinType: CoinType.FIL,
  fromAddress: mockFilecoinAccountInfo.address,
  toAddress: mockAccounts[1].address,
  sendApproveOrSellAssetContractAddress: '',
  isERC20Send: false,
  sendApproveOrSellAmount: '100'
})

const mockAvaxErc20SendTx = createMockTransactionInfo({
  chainId: BraveWallet.AVALANCHE_MAINNET_CHAIN_ID,
  coinType: CoinType.ETH,
  fromAddress: mockEthAccountInfo.address,
  toAddress: mockAccounts[1].address,
  sendApproveOrSellAssetContractAddress: mockBitcoinErc20Token.contractAddress,
  isERC20Send: true,
  sendApproveOrSellAmount: '200'
})

const mockEthErc20SendTx = createMockTransactionInfo({
  chainId: BraveWallet.MAINNET_CHAIN_ID,
  coinType: CoinType.ETH,
  fromAddress: mockEthAccountInfo.address,
  toAddress: mockAccounts[1].address,
  sendApproveOrSellAssetContractAddress: mockBasicAttentionToken.contractAddress,
  isERC20Send: true,
  sendApproveOrSellAmount: '100'
})

describe('api slice: useGetTransactionsQuery', () => {
  it('should fetch & cache transaction infos for given data', async () => {
    const store = createMockStore({}, {
      transactionInfos: [
        mockSolanaSendTokenTx,
        // this tx should be the only one returned
        mockEthErc20SendTx,
      ]
    })

    const { result, waitForValueToChange } = renderHook(
      () =>
        useGetTransactionsQuery({
          address: mockEthErc20SendTx.fromAddress,
          coinType: CoinType.ETH,
          chainId: null
        }),
      renderHookOptionsWithCustomStore(store)
    )

    await waitForValueToChange(() => result.current.isLoading)
    const { data: txs, isLoading, error } = result.current

    expect(isLoading).toBe(false)
    expect(error).not.toBeDefined()
    expect(txs).toBeDefined()
    expect(txs?.[0]).toBeDefined()
    expect(txs?.[0].chainId).toEqual(BraveWallet.MAINNET_CHAIN_ID)
    expect(txs?.length).toEqual(1)
    expect(txs?.[0].id).toEqual(mockEthErc20SendTx.id)
    expect(txs?.[0].fromAddress).toEqual(mockEthErc20SendTx.fromAddress)
  })
  it('should fetch all transaction infos for all accounts when all filters are null', async () => {
    const store = createMockStore({}, {
      transactionInfos: [
        // all txs should be returned
        mockSolanaSendTokenTx,
        mockAvaxErc20SendTx,
        mockEthErc20SendTx,
        mockFilSendTx,
      ]
    })

    const { result, waitForValueToChange } = renderHook(
      () =>
        useGetTransactionsQuery({
          address: null,
          coinType: null,
          chainId: null
        }),
      renderHookOptionsWithCustomStore(store)
    )

    await waitForValueToChange(() => result.current.isLoading)
    const { data: txs = [], isLoading, error } = result.current

    const txIds = txs?.map(({ id }) => id)

    expect(isLoading).toBe(false)
    expect(error).not.toBeDefined()
    expect(txs).toBeDefined()
    expect(txs.length).toEqual(4)
    expect(txIds).toContain(mockSolanaSendTokenTx.id)
    expect(txIds).toContain(mockAvaxErc20SendTx.id)
    expect(txIds).toContain(mockEthErc20SendTx.id)
    expect(txIds).toContain(mockFilSendTx.id)
  })
})
