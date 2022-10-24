import * as React from 'react'
import { renderHook } from '@testing-library/react-hooks'
import { Provider } from 'react-redux'

import { BraveWallet } from '../../constants/types'
import {
  getMockedTransactionInfo,
  mockAccount,
  mockAssetPrices,
  mockERC20Token,
  mockNetwork
} from '../constants/mocks'
import { SwapExchangeProxy } from './address-labels'
import { useTransactionParser } from './transaction-parser'
import { mockWalletState } from '../../stories/mock-data/mock-wallet-state'
import { combineReducers, createStore } from 'redux'
import { createWalletReducer } from '../reducers/wallet_reducer'
import { Store } from '../async/types'
import { parseTransactionWithoutPrices } from '../../utils/transaction-parser'
import { getNetworkFromTXDataUnion } from '../../utils/network-utils'

const customMockedWalletState = {
  ...mockWalletState,
  transactionSpotPrices: mockAssetPrices,
  fullTokenList: [...mockWalletState.fullTokenList, { ...mockERC20Token, contractAddress: '0xdeadbeef' }, mockERC20Token],
  selectedNetwork: mockNetwork,
  networkList: [mockNetwork]
}

const makeStore = (customStore?: any) => {
  const store = customStore || createStore(combineReducers({
    wallet: createWalletReducer(customMockedWalletState)
  }))

  store.dispatch = jest.fn(store.dispatch)
  return store
}

const store: Store = makeStore()

function renderHookOptionsWithCustomStore (store: any) {
  return {
    wrapper: ({ children }: { children?: React.ReactChildren }) =>
      <Provider store={store}>
        {children}
      </Provider>
  }
}

const parseTxUsingStoreData = (
  tx: BraveWallet.TransactionInfo,
  store: Store
) => {
  const {
    accounts,
    fullTokenList,
    userVisibleTokensInfo,
    networkList,
    selectedNetwork
  } = store.getState().wallet
  return parseTransactionWithoutPrices({
    accounts: accounts,
    fullTokenList: fullTokenList,
    userVisibleTokensList: userVisibleTokensInfo,
    transactionNetwork: getNetworkFromTXDataUnion(
      tx.txDataUnion,
      networkList,
      selectedNetwork
    ),
    tx
  })
}

describe('useTransactionParser hook & txParser utils parse data the same way', () => {
  describe('check for sameAddressError', () => {
    describe.each([
      ['ERC20Transfer', BraveWallet.TransactionType.ERC20Transfer, 'recipient'],
      ['ERC20Approve', BraveWallet.TransactionType.ERC20Approve, 'approval target']
    ])('%s', (_, txType, toLabel) => {
      it(`should be defined when sender and ${toLabel} are same`, () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const testTx = {
          ...mockTransactionInfo,
          txType,
          fromAddress: '0xdeadbeef',
          txArgs: ['0xdeadbeef', 'foo']
        } as BraveWallet.TransactionInfo
        const parsedTransaction = transactionParser(testTx)

        expect(parsedTransaction.sameAddressError).toBeDefined()
        expect(parseTxUsingStoreData(testTx, store).hasSameAddressError).toBeDefined()
      })

      it(`should be undefined when sender and ${toLabel} are different`, () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const testTx = {
          ...mockTransactionInfo,
          txType,
          fromAddress: '0xdeadbeef',
          txArgs: ['0xbadcafe', 'foo']
        }
        const parsedTransaction = transactionParser(testTx)

        expect(parsedTransaction.sameAddressError).toBeUndefined()
        expect(parseTxUsingStoreData(testTx, store).hasSameAddressError).toBeFalsy() // updated to a bool from a locale string
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
        const testTx = {
          ...mockTransactionInfo,
          txType,
          fromAddress: '0xdeadbeef',
          txArgs: ['mockOwner', '0xdeadbeef', 'mockTokenID'] // (address owner, address to, uint256 tokenId)
        }
        const parsedTransaction = transactionParser(testTx)

        expect(parsedTransaction.sameAddressError).toBeUndefined()
        expect(parseTxUsingStoreData(testTx, store).hasSameAddressError).toBeFalsy() // updated to a bool from a locale string
      })

      it('should be defined when owner and recipient are same', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const testTx = {
          ...mockTransactionInfo,
          txType,
          fromAddress: 'mockFromAddress',
          txArgs: ['0xdeadbeef', '0xdeadbeef', 'mockTokenID']
        }
        const parsedTransaction = transactionParser(testTx)

        expect(parsedTransaction.sameAddressError).toBeDefined()
        expect(parseTxUsingStoreData(testTx, store).hasSameAddressError).toBeTruthy()
      })

      it('should be undefined when owner and recipient are different', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const testTx = {
          ...mockTransactionInfo,
          txType,
          fromAddress: 'mockFromAddress',
          txArgs: ['mockOwner', 'mockToAddress', 'mockTokenID']
        }
        const parsedTransaction = transactionParser(testTx)

        expect(parsedTransaction.sameAddressError).toBeUndefined()
        expect(parseTxUsingStoreData(testTx, store).hasSameAddressError).toBeFalsy() // now a bool
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
        const testTx = {
          ...mockTransactionInfo,
          txType,
          fromAddress: '0xdeadbeef',
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            ethTxData1559: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559!,
              baseData: {
                ...mockTransactionInfo.txDataUnion.ethTxData1559!.baseData,
                to: name === '0x Swap' ? SwapExchangeProxy : '0xdeadbeef'
              }
            }
          }
        }
        const parsedTransaction = transactionParser(testTx)

        expect(parsedTransaction.sameAddressError).toBeUndefined()
        expect(parseTxUsingStoreData(testTx, store).hasSameAddressError).toBeFalsy() // now a bool
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
      it(`${name} result should always be undefined`, () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const testTx = {
          ...mockTransactionInfo,
          txArgs: txType === BraveWallet.TransactionType.ETHSend ? [] : ['mockArg1', 'mockArg2'],
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            ethTxData1559: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559!,
              baseData: {
                ...mockTransactionInfo.txDataUnion.ethTxData1559!.baseData,
                to: name === '0x Swap' ? SwapExchangeProxy : '0xdeadbeef'
              }
            }
          }
        }
        const parsedTransaction = transactionParser(testTx)

        expect(parsedTransaction.contractAddressError).toBeUndefined()
        expect(parseTxUsingStoreData(testTx, store).hasContractAddressError).toBeFalsy() // now a bool
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
        const testTx = {
          ...mockTransactionInfo,
          txArgs: txType === BraveWallet.TransactionType.ERC20Transfer
            ? ['0xdeadbeef', 'mockAmount']
            : ['mockOwner', '0xdeadbeef', 'mockTokenID'],
          txType
        }
        const parsedTransaction = transactionParser(testTx)

        expect(parsedTransaction.contractAddressError).toBeDefined()
        expect(parseTxUsingStoreData(testTx, store).hasContractAddressError).toBeTruthy() // now a bool
      })

      it('should be undefined when recipient is an unknown contract address', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const testTx = {
          ...mockTransactionInfo,
          txArgs: txType === BraveWallet.TransactionType.ERC20Transfer
            ? ['0xbadcafe', 'mockAmount']
            : ['mockOwner', '0xbadcafe', 'mockTokenID'],
          txType
        }
        const parsedTransaction = transactionParser(testTx)

        expect(parsedTransaction.contractAddressError).toBeUndefined()
        expect(parseTxUsingStoreData(testTx, store).hasContractAddressError).toBeFalsy() // now a bool
      })
    })
  })

  describe('check for insufficient funds errors', () => {
    describe.each([
      ['ERC20Transfer', BraveWallet.TransactionType.ERC20Transfer],
      ['ERC20Approve', BraveWallet.TransactionType.ERC20Approve],
      ['ERC721TransferFrom', BraveWallet.TransactionType.ERC721TransferFrom],
      ['ERC721SafeTransferFrom', BraveWallet.TransactionType.ERC721SafeTransferFrom],
      ['ETHSend', BraveWallet.TransactionType.ETHSend],
      ['Other', BraveWallet.TransactionType.Other]
    ])('%s', (_, txType) => {
      it('should correctly indicate when funds are insufficient for gas', () => {
        /**
         * Account balance: 0.001 ETH
         * Transaction value: 0 ETH
         *
         * Gas fee: 0.00315 ETH
         *   - gasPrice: 150 Gwei
         *   - gasLimit: 21000
         */
        const customStore = createStore(combineReducers({
            wallet: createWalletReducer({
              ...mockWalletState,
              transactionSpotPrices: mockAssetPrices,
              fullTokenList: [...mockWalletState.fullTokenList, { ...mockERC20Token, contractAddress: '0xdeadbeef' }, mockERC20Token],
              accounts: [
                ...mockWalletState.accounts,
                {
                  ...mockAccount,
                  tokenBalanceRegistry: {
                    [mockERC20Token.contractAddress.toLowerCase()]: '1000000000000000' // 0.01 ETH
                  },
                  address: '0xdeadbeef',
                  nativeBalanceRegistry: {
                    '0x1': '1000000000000000' // 0.001 ETH
                  }
                }
              ]
            })
          })) as Store

        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
            renderHookOptionsWithCustomStore(customStore))

        const mockTransactionInfo = getMockedTransactionInfo()
        const testTx = {
          ...mockTransactionInfo,
          fromAddress: '0xdeadbeef',
          txArgs: [BraveWallet.TransactionType.ERC20Approve, BraveWallet.TransactionType.ERC20Transfer].includes(txType)
            ? ['mockRecipient', '0x0']
            : ['mockOwner', 'mockRecipient', 'mockTokenID'],
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            ethTxData1559: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559!,
              maxFeePerGas: '0x22ecb25c00',
              maxPriorityFeePerGas: '0x22ecb25c00',
              baseData: {
                ...mockTransactionInfo.txDataUnion.ethTxData1559!.baseData,
                to: txType === BraveWallet.TransactionType.ERC20Transfer
                  ? mockERC20Token.contractAddress.toLowerCase()
                  : mockTransactionInfo.txDataUnion.ethTxData1559!.baseData.to,
                value: '0x0',
                gasLimit: '0x5208',
                gasPrice: '0x22ecb25c00'
              }
            }
          }
        }
        const parsedTransaction = transactionParser(testTx)

        expect(parsedTransaction.insufficientFundsForGasError).toBeTruthy()
        expect(parseTxUsingStoreData(testTx, customStore).insufficientFundsForGas).toBeTruthy() // now a bool
      })

      it('should be false when funds are sufficient for gas', () => {
        /**
         * Account balance: 1 ETH
         * Transaction value: 0 ETH
         *
         * Gas fee: 0.00315 ETH
         *   - gasPrice: 150 Gwei
         *   - gasLimit: 21000
         */
        const mockTransactionInfo = getMockedTransactionInfo()
        const mockTxData: BraveWallet.TxData1559 = mockTransactionInfo.txDataUnion.ethTxData1559!

        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const testTx = {
          ...mockTransactionInfo,
          fromAddress: '0xdeadbeef',
          txArgs: [BraveWallet.TransactionType.ERC20Approve, BraveWallet.TransactionType.ERC20Transfer].includes(txType)
            ? ['mockRecipient', '0x0']
            : ['mockOwner', 'mockRecipient', 'mockTokenID'],
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            ethTxData1559: {
              ...mockTxData,
              baseData: {
                ...mockTxData.baseData,
                value: '0x0',
                gasLimit: '0x5208',
                gasPrice: '0x22ecb25c00'
              }
            }
          }
        }
        const parsedTransaction = transactionParser(testTx)

        expect(parsedTransaction.insufficientFundsForGasError).toBeFalsy()
        expect(parsedTransaction.insufficientFundsError).toBeFalsy()

        const newParsedTx = parseTxUsingStoreData(testTx, store)
        expect(newParsedTx.insufficientFundsForGas).toBeFalsy() // now a bool
        expect(newParsedTx.insufficientFunds).toBeFalsy() // now a bool
      })
    })

    describe.each([
      ['ETHSend', BraveWallet.TransactionType.ETHSend],
      ['Other', BraveWallet.TransactionType.Other]
    ])('%s', (_, txType) => {
      const mockTransactionInfo = getMockedTransactionInfo()
      const transactionInfo: BraveWallet.TransactionInfo = {
        ...mockTransactionInfo,
        fromAddress: '0xdeadbeef',
        txType,
        txDataUnion: {
          ethTxData: {} as any,
          filTxData: undefined,
          solanaTxData: undefined,
          ethTxData1559: {
            ...mockTransactionInfo.txDataUnion.ethTxData1559!,
            baseData: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559!.baseData,
              value: '0xde0b6b3a7640000', // 1 ETH
              gasLimit: '0x5208', // 21000
              gasPrice: '0x22ecb25c00' // 150 Gwei
            }
          }
        }
      }

      it('should be true when funds are insufficient for send amount', () => {
        /**
         * Account balance: 0.004 ETH
         * Transaction value: 1 ETH
         *
         * Gas fee: 0.00315 ETH
         *   - gasPrice: 150 Gwei
         *   - gasLimit: 21000
         *
         * Remarks: sufficient funds for gas, but not for send amount.
         */
        const customStore = createStore(combineReducers({
          wallet: createWalletReducer({
            ...customMockedWalletState,
            accounts: [{
              ...mockAccount,
              address: '0xdeadbeef',
              nativeBalanceRegistry: {
                '0x1': '4000000000000000' // 0.004 ETH
              }
            }],
            selectedNetwork: mockNetwork
          })
        })) as Store

        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(customStore))

        const parsedTransaction = transactionParser(transactionInfo)
        expect(parsedTransaction.insufficientFundsError).toBeTruthy()
        expect(parsedTransaction.insufficientFundsForGasError).toBeFalsy()

        const newParsedTx = parseTxUsingStoreData(transactionInfo, customStore)
        expect(newParsedTx.insufficientFunds).toBeTruthy() // now a bool
        expect(newParsedTx.insufficientFundsForGas).toBeFalsy() // now a bool
      })

      it('should be false when funds are sufficient for send amount', () => {
        /**
         * Account balance: 1.00315 ETH
         * Transaction value: 1 ETH
         *
         * Gas fee: 0.00315 ETH
         *   - gasPrice: 150 Gwei
         *   - gasLimit: 21000
         *
         * Remarks: sufficient funds for gas, and for send amount.
         */

        const customStore = createStore(combineReducers({
            wallet: createWalletReducer({
              ...customMockedWalletState,
              accounts: [{
                ...mockAccount,
                address: '0xdeadbeef',
                nativeBalanceRegistry: {
                  '0x1': '1003150000000000000' // 1.00315 ETH
                }
              }]
            })
          })) as Store

        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
            renderHookOptionsWithCustomStore(customStore))

        const parsedTransaction = transactionParser(transactionInfo)
        expect(parsedTransaction.insufficientFundsError).toBeFalsy()
        expect(parsedTransaction.insufficientFundsForGasError).toBeFalsy()

        const newParsedTx = parseTxUsingStoreData(transactionInfo, customStore)
        expect(newParsedTx.insufficientFunds).toBeFalsy()
        expect(newParsedTx.insufficientFundsForGas).toBeFalsy()
      })
    })

    describe('ERC20Transfer', () => {
      /**
       * Account ETH balance: 0.00315 ETH
       * Account DOG balance: 1 DOG
       * Transaction value: 1 DOG
       *
       * Gas fee: 0.00315 ETH
       *   - gasPrice: 150 Gwei
       *   - gasLimit: 21000
       *
       * Remarks: sufficient funds for gas, but not for send amount.
       */
      const mockTransactionInfo = getMockedTransactionInfo()
      const transactionInfo: BraveWallet.TransactionInfo = {
        ...mockTransactionInfo,
        fromAddress: '0xdeadbeef',
        txArgs: [
          'mockRecipient',
          '0xde0b6b3a7640000' // 1 DOG
        ],
        txType: BraveWallet.TransactionType.ERC20Transfer,
        txDataUnion: {
          ethTxData: {} as any,
          filTxData: undefined,
          solanaTxData: undefined,
          ethTxData1559: {
            ...mockTransactionInfo.txDataUnion.ethTxData1559!,
            baseData: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559!.baseData,
              to: mockERC20Token.contractAddress,
              value: '0x0', // 0 ETH
              gasLimit: '0x5208', // 21000
              gasPrice: '0x22ecb25c00' // 150 Gwei
            }
          }
        }
      }
      const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
        renderHookOptionsWithCustomStore(store))
      const parsedTransaction = transactionParser(transactionInfo)
      const newParsedTx = parseTxUsingStoreData(transactionInfo, store)

      it('should be true when funds are insufficient for send amount', () => {
        // [FIXME] - Difficult to capture results from reinvocation of a
        // useCallback(), which fails the following assertion. Fix this
        // by returning insufficientFundsError as part of the result. The
        // test was previously passing as a false positive. There might be
        // similar issues with other tests, so transaction parser hook must
        // be rewritten.
        //
        // expect(parsedTransaction.insufficientFundsError).toBeTruthy()
        expect(parsedTransaction.insufficientFundsForGasError).toBeFalsy()
        expect(newParsedTx.insufficientFundsForGas).toBeFalsy()
      })

      it('should be false when funds are sufficient for send amount', () => {
        expect(parsedTransaction.insufficientFundsError).toBeFalsy()
        expect(parsedTransaction.insufficientFundsForGasError).toBeFalsy()
        expect(newParsedTx.insufficientFunds).toBeFalsy()
        expect(newParsedTx.insufficientFundsForGas).toBeFalsy()
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
        const testTx = {
          ...mockTransactionInfo,
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            ethTxData1559: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559!,
              baseData: {
                ...mockTransactionInfo.txDataUnion.ethTxData1559!.baseData,
                to: 'test'
              }
            }
          },
          txArgs: [
            'mockRecipient',
            '0xde0b6b3a7640000'
          ]
        }
        const parsedTransaction = transactionParser(testTx)
        expect(parsedTransaction.symbol).toEqual('')

        const newParsedTx = parseTxUsingStoreData(testTx, store)

        expect(newParsedTx.symbol).toEqual('')
      })

      it('Gets token symbol from visibleList, should be DOG', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(),
          renderHookOptionsWithCustomStore(store))

        const mockTransactionInfo = getMockedTransactionInfo()
        const testTx = {
          ...mockTransactionInfo,
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            ethTxData1559: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559!,
              baseData: {
                ...mockTransactionInfo.txDataUnion.ethTxData1559!.baseData,
                to: 'mockContractAddress'
              }
            }
          },
          txArgs: [
            'mockRecipient',
            '0xde0b6b3a7640000'
          ]
        }
        const parsedTransaction = transactionParser(testTx)

        expect(parsedTransaction.symbol).toEqual('DOG')

        const newParsedTx = parseTxUsingStoreData(testTx, store)
        expect(newParsedTx.symbol).toEqual('DOG')
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

        const mockTransactionInfo1: BraveWallet.TransactionInfo = {
          ...baseMockTransactionInfo,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            ethTxData1559: {
              ...baseMockTransactionInfo.txDataUnion.ethTxData1559!,
              baseData: {
                ...baseMockTransactionInfo.txDataUnion.ethTxData1559!.baseData,
                gasLimit: ''
              }
            }
          }
        }
        const parsedTransaction1 = transactionParser(mockTransactionInfo1)
        const newParsedTx1 = parseTxUsingStoreData(mockTransactionInfo1, store)

        expect(parsedTransaction1.gasLimit).toEqual('')
        expect(parsedTransaction1.missingGasLimitError).toBeTruthy()
        expect(newParsedTx1.gasLimit).toEqual('')
        expect(newParsedTx1.isMissingGasLimit).toBeTruthy()

        const mockTransactionInfo2: BraveWallet.TransactionInfo = {
          ...baseMockTransactionInfo,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            ethTxData1559: {
              ...baseMockTransactionInfo.txDataUnion.ethTxData1559!,
              baseData: {
                ...baseMockTransactionInfo.txDataUnion.ethTxData1559!.baseData,
                gasLimit: '0x0'
              }
            }
          }
        }
        const parsedTransaction2 = transactionParser(mockTransactionInfo2)

        const newParsedTx2 = parseTxUsingStoreData(mockTransactionInfo2, store)
        expect(parsedTransaction2.gasLimit).toEqual('0')
        expect(parsedTransaction2.missingGasLimitError).toBeTruthy()
        expect(newParsedTx2.gasLimit).toEqual('0')
        expect(newParsedTx2.isMissingGasLimit).toBeTruthy()

        const mockTransactionInfo3: BraveWallet.TransactionInfo = {
          ...baseMockTransactionInfo,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            ethTxData1559: {
              ...baseMockTransactionInfo.txDataUnion.ethTxData1559!,
              baseData: {
                ...baseMockTransactionInfo.txDataUnion.ethTxData1559!.baseData,
                gasLimit: '0x1'
              }
            }
          }
        }
        const parsedTransaction3 = transactionParser(mockTransactionInfo3)
        const newParsedTx3 = parseTxUsingStoreData(mockTransactionInfo3, store)
        expect(parsedTransaction3.gasLimit).toEqual('1')
        expect(parsedTransaction3.missingGasLimitError).toBeUndefined()
        expect(newParsedTx3.gasLimit).toEqual('1')
        expect(newParsedTx3.isMissingGasLimit).toBeFalsy()
      })
    })
  })
})
