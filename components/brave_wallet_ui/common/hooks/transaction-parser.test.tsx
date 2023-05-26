// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { renderHook } from '@testing-library/react-hooks'
import { Provider } from 'react-redux'

import {
  BraveWallet,
  SerializableTransactionInfo,
  WalletState
} from '../../constants/types'
import {
  getMockedTransactionInfo,
  mockAssetPrices,
  mockERC20Token,
} from '../constants/mocks'
import { useTransactionParser } from './transaction-parser'
import { mockWalletState } from '../../stories/mock-data/mock-wallet-state'
import { SwapExchangeProxy } from '../constants/registry'
import { createMockStore } from '../../utils/test-utils'

const customMockedWalletState: WalletState = {
  ...mockWalletState,
  transactionSpotPrices: mockAssetPrices,
  fullTokenList: [
    ...mockWalletState.fullTokenList,
    { ...mockERC20Token, contractAddress: '0xdeadbeef' },
    mockERC20Token
  ]
}

const makeStore = (customStore?: any) => {
  const store = customStore || createMockStore({
    walletStateOverride: customMockedWalletState
  })

  store.dispatch = jest.fn(store.dispatch)
  return store
}

const store = makeStore()

function renderHookOptionsWithCustomStore (store: any) {
  return {
    wrapper: ({ children }: { children?: React.ReactChildren }) =>
      <Provider store={store}>
        {children}
      </Provider>
  }
}

describe('useTransactionParser hook', () => {
  describe('check for sameAddressError', () => {
    describe.each([
      ['ERC20Transfer', BraveWallet.TransactionType.ERC20Transfer, 'recipient'],
      ['ERC20Approve', BraveWallet.TransactionType.ERC20Approve, 'approval target']
    ])('%s', (_, txType, toLabel) => {
      it(`should be defined when sender and ${toLabel} are same`, () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          fromAddress: '0xdeadbeef',
          txArgs: ['0xdeadbeef', 'foo']
        })

        expect(parsedTransaction.sameAddressError).toBeDefined()
      })

      it(`should be undefined when sender and ${toLabel} are different`, () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          fromAddress: '0xdeadbeef',
          txArgs: ['0xbadcafe', 'foo']
        })

        expect(parsedTransaction.sameAddressError).toBeUndefined()
      })
    })

    describe.each([
      ['ERC721TransferFrom', BraveWallet.TransactionType.ERC721TransferFrom],
      ['ERC721SafeTransferFrom', BraveWallet.TransactionType.ERC721SafeTransferFrom]
    ])('%s', (_, txType) => {
      it('should be undefined when sender and recipient are same', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          fromAddress: '0xdeadbeef',
          txArgs: ['mockOwner', '0xdeadbeef', 'mockTokenID']
        })

        expect(parsedTransaction.sameAddressError).toBeUndefined()
      })

      it('should be defined when owner and recipient are same', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          fromAddress: 'mockFromAddress',
          txArgs: ['0xdeadbeef', '0xdeadbeef', 'mockTokenID']
        })

        expect(parsedTransaction.sameAddressError).toBeDefined()
      })

      it('should be undefined when owner and recipient are different', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          fromAddress: 'mockFromAddress',
          txArgs: ['mockOwner', 'mockToAddress', 'mockTokenID']
        })

        expect(parsedTransaction.sameAddressError).toBeUndefined()
      })
    })

    describe.each([
      // ETHSend can have same sender and recipient in case of cancel transactions
      ['ETHSend', BraveWallet.TransactionType.ETHSend],
      ['Other', BraveWallet.TransactionType.Other],
      ['0x Swap', BraveWallet.TransactionType.Other]
    ])('%s', (name, txType) => {
      it('should always be undefined', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          fromAddress: '0xdeadbeef',
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            ethTxData1559: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559,
              baseData: {
                ...mockTransactionInfo.txDataUnion.ethTxData1559.baseData,
                to: name === '0x Swap' ? SwapExchangeProxy : '0xdeadbeef'
              }
            }
          }
        })

        expect(parsedTransaction.sameAddressError).toBeUndefined()
      })
    })
  })

  describe('check for contractAddressError', () => {
    describe.each([
      ['ERC20Approve', BraveWallet.TransactionType.ERC20Approve],
      ['ETHSend', BraveWallet.TransactionType.ETHSend],
      ['Other', BraveWallet.TransactionType.Other],
      ['0x Swap', BraveWallet.TransactionType.Other]
    ])('%s', (name, txType) => {
      it('should always be undefined', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txArgs: txType === BraveWallet.TransactionType.ETHSend ? [] : ['mockArg1', 'mockArg2'],
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            ethTxData1559: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559,
              baseData: {
                ...mockTransactionInfo.txDataUnion.ethTxData1559.baseData,
                to: name === '0x Swap' ? SwapExchangeProxy : '0xdeadbeef'
              }
            }
          }
        })

        expect(parsedTransaction.contractAddressError).toBeUndefined()
      })
    })

    describe.each([
      ['ERC20Transfer', BraveWallet.TransactionType.ERC20Transfer],
      ['ERC721TransferFrom', BraveWallet.TransactionType.ERC721TransferFrom],
      ['ERC721SafeTransferFrom', BraveWallet.TransactionType.ERC721SafeTransferFrom]
    ])('%s', (_, txType) => {
      it('should be defined when recipient is a known contract address', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txArgs: txType === BraveWallet.TransactionType.ERC20Transfer
            ? ['0xdeadbeef', 'mockAmount']
            : ['mockOwner', '0xdeadbeef', 'mockTokenID'],
          txType
        })

        expect(parsedTransaction.contractAddressError).toBeDefined()
      })

      it('should be undefined when recipient is an unknown contract address', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txArgs: txType === BraveWallet.TransactionType.ERC20Transfer
            ? ['0xbadcafe', 'mockAmount']
            : ['mockOwner', '0xbadcafe', 'mockTokenID'],
          txType
        })

        expect(parsedTransaction.contractAddressError).toBeUndefined()
      })
    })
  })

  describe('check for token symbol', () => {
    describe.each([
      ['ERC20Approve', BraveWallet.TransactionType.ERC20Approve],
      ['ERC20Transfer', BraveWallet.TransactionType.ERC20Transfer],
      ['ERC721TransferFrom', BraveWallet.TransactionType.ERC721TransferFrom],
      ['ERC721SafeTransferFrom', BraveWallet.TransactionType.ERC721SafeTransferFrom]
    ])('%s', (_, txType) => {
      it('should be empty', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            ethTxData1559: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559,
              baseData: {
                ...mockTransactionInfo.txDataUnion.ethTxData1559.baseData,
                to: 'test'
              }
            }
          },
          txArgs: [
            'mockRecipient',
            '0xde0b6b3a7640000'
          ]
        })

        expect(parsedTransaction.symbol).toEqual('')
      })

      it('Gets token symbol from visibleList, should be DOG', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            ethTxData1559: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559,
              baseData: {
                ...mockTransactionInfo.txDataUnion.ethTxData1559.baseData,
                to: 'mockContractAddress'
              }
            }
          },
          txArgs: [
            'mockRecipient',
            '0xde0b6b3a7640000'
          ]
        })

        expect(parsedTransaction.symbol).toEqual('DOG')
      })
    })
  })

  describe('check for empty gas limit', () => {
    describe.each([
      ['ERC20Approve', BraveWallet.TransactionType.ERC20Approve],
      ['ERC20Transfer', BraveWallet.TransactionType.ERC20Transfer],
      ['ERC721TransferFrom', BraveWallet.TransactionType.ERC721TransferFrom],
      ['ERC721SafeTransferFrom', BraveWallet.TransactionType.ERC721SafeTransferFrom],
      ['ETHSend', BraveWallet.TransactionType.ETHSend],
      ['Other', BraveWallet.TransactionType.Other]
    ])('%s', (_, txType) => {
      it('should return missingGasLimitError if gas limit is zero or empty', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const baseMockTransactionInfo = {
          ...getMockedTransactionInfo(),
          txType,
          txArgs: [
            'mockRecipient',
            'mockAmount'
          ]
        }

        const mockTransactionInfo1: SerializableTransactionInfo = {
          ...baseMockTransactionInfo,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            ethTxData1559: {
              ...baseMockTransactionInfo.txDataUnion.ethTxData1559,
              baseData: {
                ...baseMockTransactionInfo.txDataUnion.ethTxData1559.baseData,
                gasLimit: ''
              }
            }
          }
        }
        const parsedTransaction1 = transactionParser(mockTransactionInfo1)
        expect(parsedTransaction1.gasLimit).toEqual('')
        expect(parsedTransaction1.missingGasLimitError).toBeTruthy()

        const mockTransactionInfo2: SerializableTransactionInfo = {
          ...baseMockTransactionInfo,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            ethTxData1559: {
              ...baseMockTransactionInfo.txDataUnion.ethTxData1559,
              baseData: {
                ...baseMockTransactionInfo.txDataUnion.ethTxData1559.baseData,
                gasLimit: '0x0'
              }
            }
          }
        }
        const parsedTransaction2 = transactionParser(mockTransactionInfo2)
        expect(parsedTransaction2.gasLimit).toEqual('0')
        expect(parsedTransaction2.missingGasLimitError).toBeTruthy()

        const mockTransactionInfo3: SerializableTransactionInfo = {
          ...baseMockTransactionInfo,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            ethTxData1559: {
              ...baseMockTransactionInfo.txDataUnion.ethTxData1559,
              baseData: {
                ...baseMockTransactionInfo.txDataUnion.ethTxData1559.baseData,
                gasLimit: '0x1'
              }
            }
          }
        }
        const parsedTransaction3 = transactionParser(mockTransactionInfo3)
        expect(parsedTransaction3.gasLimit).toEqual('1')
        expect(parsedTransaction3.missingGasLimitError).toBeUndefined()
      })
    })
  })
})
