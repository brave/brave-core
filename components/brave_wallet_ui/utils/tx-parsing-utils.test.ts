// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { BraveWallet, SerializableTransactionInfo } from '../constants/types'
import { SwapExchangeProxy } from '../common/constants/registry'

// utils
import Amount from './amount'

// mocks
import {
  getMockedTransactionInfo,
  mockEthAccountInfo
} from '../common/constants/mocks'
import {
  findTransactionToken,
  getTransactionGasLimit,
  getTransactionTokenSymbol,
  isTransactionGasLimitMissing,
  transactionHasSameAddressError,
  isSendingToKnownTokenContractAddress
} from './tx-utils'
import {
  mockERC20Token,
  mockErc20TokensList
} from '../stories/mock-data/mock-asset-options'

const tokenList = [
  ...mockErc20TokensList,
  { ...mockERC20Token, contractAddress: '0xdeadbeef' },
  mockERC20Token
]

describe('Transaction Parsing utils', () => {
  describe('check for sameAddressError', () => {
    describe.each([
      ['ERC20Transfer', BraveWallet.TransactionType.ERC20Transfer, 'recipient'],
      [
        'ERC20Approve',
        BraveWallet.TransactionType.ERC20Approve,
        'approval target'
      ]
    ])('%s', (_, txType, toLabel) => {
      it(`should be truthy when sender and ${toLabel} are same`, () => {
        const mockTransaction: SerializableTransactionInfo = {
          ...getMockedTransactionInfo(),
          txType,
          txArgs: [mockEthAccountInfo.address, 'foo']
        }

        const sameAddressError = transactionHasSameAddressError(mockTransaction)

        expect(sameAddressError).toBeTruthy()
      })

      it(`should be falsey when sender and ${toLabel} are different`, () => {
        const mockTransaction: SerializableTransactionInfo = {
          ...getMockedTransactionInfo(),
          txType,
          txArgs: ['0xbadcafe', 'foo']
        }

        const sameAddressError = transactionHasSameAddressError(mockTransaction)

        expect(sameAddressError).toBeFalsy()
      })
    })

    describe.each([
      ['ERC721TransferFrom', BraveWallet.TransactionType.ERC721TransferFrom],
      [
        'ERC721SafeTransferFrom',
        BraveWallet.TransactionType.ERC721SafeTransferFrom
      ]
    ])('%s', (_, txType) => {
      it('should be undefined when sender and recipient are same', () => {
        const mockTransaction: SerializableTransactionInfo = {
          ...getMockedTransactionInfo(),
          txType,
          txArgs: ['mockOwner', mockEthAccountInfo.address, 'mockTokenID']
        }

        const sameAddressError = transactionHasSameAddressError(mockTransaction)

        expect(sameAddressError).toBeFalsy()
      })

      it('should be defined when owner and recipient are same', () => {
        const mockTransaction: SerializableTransactionInfo = {
          ...getMockedTransactionInfo(),
          txType,
          txArgs: ['0xdeadbeef', '0xdeadbeef', 'mockTokenID']
        }

        const sameAddressError = transactionHasSameAddressError(mockTransaction)

        expect(sameAddressError).toBeTruthy()
      })

      it('should be falsey when owner and recipient are different', () => {
        const mockTransaction: SerializableTransactionInfo = {
          ...getMockedTransactionInfo(),
          txType,
          txArgs: ['mockOwner', 'mockToAddress', 'mockTokenID']
        }

        const sameAddressError = transactionHasSameAddressError(mockTransaction)

        expect(sameAddressError).toBeFalsy()
      })
    })

    describe.each([
      // ETHSend can have same sender and recipient
      // in case of cancel transactions
      ['ETHSend', BraveWallet.TransactionType.ETHSend],
      ['Other', BraveWallet.TransactionType.Other],
      ['0x Swap', BraveWallet.TransactionType.Other]
    ])('%s', (name, txType) => {
      it('should always be falsey', () => {
        const mockTransactionInfo = getMockedTransactionInfo()
        const mockTransaction: SerializableTransactionInfo = {
          ...mockTransactionInfo,
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            btcTxData: undefined,
            zecTxData: undefined,
            ethTxData1559: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559,
              baseData: {
                ...mockTransactionInfo.txDataUnion.ethTxData1559.baseData,
                to:
                  name === '0x Swap'
                    ? SwapExchangeProxy
                    : mockEthAccountInfo.address
              }
            }
          }
        }

        const sameAddressError = transactionHasSameAddressError(mockTransaction)

        expect(sameAddressError).toBeFalsy()
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
      it('should always be falsey', () => {
        const mockTransactionInfo = getMockedTransactionInfo()
        const mockTransaction: SerializableTransactionInfo = {
          ...mockTransactionInfo,
          txArgs:
            txType === BraveWallet.TransactionType.ETHSend
              ? []
              : ['mockArg1', 'mockArg2'],
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            btcTxData: undefined,
            zecTxData: undefined,
            ethTxData1559: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559,
              baseData: {
                ...mockTransactionInfo.txDataUnion.ethTxData1559.baseData,
                to: name === '0x Swap' ? SwapExchangeProxy : '0xdeadbeef'
              }
            }
          }
        }

        const contractAddressError = isSendingToKnownTokenContractAddress(
          mockTransaction,
          tokenList
        )

        expect(contractAddressError).toBeFalsy()
      })
    })

    describe.each([
      ['ERC20Transfer', BraveWallet.TransactionType.ERC20Transfer],
      ['ERC721TransferFrom', BraveWallet.TransactionType.ERC721TransferFrom],
      [
        'ERC721SafeTransferFrom',
        BraveWallet.TransactionType.ERC721SafeTransferFrom
      ]
    ])('%s', (_, txType) => {
      it('should be truthy when recipient is a known contract address', () => {
        const mockTransaction: SerializableTransactionInfo = {
          ...getMockedTransactionInfo(),
          txArgs:
            txType === BraveWallet.TransactionType.ERC20Transfer
              ? ['0xdeadbeef', 'mockAmount']
              : ['mockOwner', '0xdeadbeef', 'mockTokenID'],
          txType
        }

        const contractAddressError = isSendingToKnownTokenContractAddress(
          mockTransaction,
          tokenList
        )

        expect(contractAddressError).toBeTruthy()
      })

      it(
        'should be falsey when recipient is ' + 'an unknown contract address',
        () => {
          const mockTransaction: SerializableTransactionInfo = {
            ...getMockedTransactionInfo(),
            txArgs:
              txType === BraveWallet.TransactionType.ERC20Transfer
                ? ['0xbadcafe', 'mockAmount']
                : ['mockOwner', '0xbadcafe', 'mockTokenID'],
            txType
          }

          const contractAddressError = isSendingToKnownTokenContractAddress(
            mockTransaction,
            tokenList
          )

          expect(contractAddressError).toBeFalsy()
        }
      )
    })
  })

  describe('check for token symbol', () => {
    describe.each([
      ['ERC20Approve', BraveWallet.TransactionType.ERC20Approve],
      ['ERC20Transfer', BraveWallet.TransactionType.ERC20Transfer],
      ['ERC721TransferFrom', BraveWallet.TransactionType.ERC721TransferFrom],
      [
        'ERC721SafeTransferFrom',
        BraveWallet.TransactionType.ERC721SafeTransferFrom
      ]
    ])('%s', (_, txType) => {
      it('should be empty', () => {
        const mockTransactionInfo = getMockedTransactionInfo()
        const mockTransaction: SerializableTransactionInfo = {
          ...mockTransactionInfo,
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            btcTxData: undefined,
            zecTxData: undefined,
            ethTxData1559: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559,
              baseData: {
                ...mockTransactionInfo.txDataUnion.ethTxData1559.baseData,
                to: 'test'
              }
            }
          },
          txArgs: ['mockRecipient', '0xde0b6b3a7640000']
        }

        const token = findTransactionToken(mockTransaction, tokenList)

        const txSymbol = getTransactionTokenSymbol({
          tx: mockTransaction,
          sellToken: undefined,
          token,
          txNetwork: { symbol: 'ETH' }
        })

        expect(txSymbol).toEqual('')
      })

      it('Gets token symbol from visibleList, should be DOG', () => {
        const mockTransactionInfo = getMockedTransactionInfo()
        const mockTransaction: SerializableTransactionInfo = {
          ...mockTransactionInfo,
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: undefined,
            btcTxData: undefined,
            zecTxData: undefined,
            ethTxData1559: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559,
              baseData: {
                ...mockTransactionInfo.txDataUnion.ethTxData1559.baseData,
                to: 'mockContractAddress'
              }
            }
          },
          txArgs: ['mockRecipient', '0xde0b6b3a7640000']
        }

        const token = findTransactionToken(mockTransaction, tokenList)

        const txSymbol = getTransactionTokenSymbol({
          tx: mockTransaction,
          sellToken: undefined,
          token,
          txNetwork: { symbol: 'ETH' }
        })

        expect(txSymbol).toEqual('DOG')
      })
    })
  })

  describe('check for empty gas limit', () => {
    describe.each([
      ['ERC20Approve', BraveWallet.TransactionType.ERC20Approve],
      ['ERC20Transfer', BraveWallet.TransactionType.ERC20Transfer],
      ['ERC721TransferFrom', BraveWallet.TransactionType.ERC721TransferFrom],
      [
        'ERC721SafeTransferFrom',
        BraveWallet.TransactionType.ERC721SafeTransferFrom
      ],
      ['ETHSend', BraveWallet.TransactionType.ETHSend],
      ['Other', BraveWallet.TransactionType.Other]
    ])('%s', (_, txType) => {
      it(
        'should return missingGasLimitError ' + 'if gas limit is zero or empty',
        () => {
          const baseMockTransactionInfo = {
            ...getMockedTransactionInfo(),
            txType,
            txArgs: ['mockRecipient', 'mockAmount']
          }

          const mockTx1: SerializableTransactionInfo = {
            ...baseMockTransactionInfo,
            txDataUnion: {
              ethTxData: {} as any,
              filTxData: undefined,
              solanaTxData: undefined,
              btcTxData: undefined,
              zecTxData: undefined,
              ethTxData1559: {
                ...baseMockTransactionInfo.txDataUnion.ethTxData1559,
                baseData: {
                  ...baseMockTransactionInfo.txDataUnion.ethTxData1559.baseData,
                  gasLimit: ''
                }
              }
            }
          }

          expect(getTransactionGasLimit(mockTx1)).toEqual('')
          expect(isTransactionGasLimitMissing(mockTx1)).toBeTruthy()

          const mockTx2: SerializableTransactionInfo = {
            ...baseMockTransactionInfo,
            txDataUnion: {
              ethTxData: {} as any,
              filTxData: undefined,
              solanaTxData: undefined,
              btcTxData: undefined,
              zecTxData: undefined,
              ethTxData1559: {
                ...baseMockTransactionInfo.txDataUnion.ethTxData1559,
                baseData: {
                  ...baseMockTransactionInfo.txDataUnion.ethTxData1559.baseData,
                  gasLimit: '0x0'
                }
              }
            }
          }

          expect(getTransactionGasLimit(mockTx2)).toEqual(
            mockTx2.txDataUnion.ethTxData1559?.baseData.gasLimit
          )
          expect(isTransactionGasLimitMissing(mockTx2)).toBeTruthy()

          const mockTx3: SerializableTransactionInfo = {
            ...baseMockTransactionInfo,
            txDataUnion: {
              ethTxData: {} as any,
              filTxData: undefined,
              solanaTxData: undefined,
              btcTxData: undefined,
              zecTxData: undefined,
              ethTxData1559: {
                ...baseMockTransactionInfo.txDataUnion.ethTxData1559,
                baseData: {
                  ...baseMockTransactionInfo.txDataUnion.ethTxData1559.baseData,
                  gasLimit: '0x1'
                }
              }
            }
          }

          expect(getTransactionGasLimit(mockTx3)).toEqual(
            mockTx3.txDataUnion.ethTxData1559?.baseData.gasLimit
          )
          expect(Amount.normalize(getTransactionGasLimit(mockTx3))).toEqual('1')
          expect(isTransactionGasLimitMissing(mockTx3)).toBeFalsy()
        }
      )
    })
  })
})
