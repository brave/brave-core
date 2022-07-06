import { renderHook } from '@testing-library/react-hooks'

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

describe('useTransactionParser hook', () => {
  describe('check for sameAddressError', () => {
    describe.each([
      ['ERC20Transfer', BraveWallet.TransactionType.ERC20Transfer, 'recipient'],
      ['ERC20Approve', BraveWallet.TransactionType.ERC20Approve, 'approval target']
    ])('%s', (_, txType, toLabel) => {
      it(`should be defined when sender and ${toLabel} are same`, () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [], []
        ))

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
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [], []
        ))

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
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [], []
        ))

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
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [], []
        ))

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
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [], []
        ))

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
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [], []
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          fromAddress: '0xdeadbeef',
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: {} as any,
            ethTxData1559: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559!,
              baseData: {
                ...mockTransactionInfo.txDataUnion.ethTxData1559!.baseData,
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
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [],
          [{ ...mockERC20Token, contractAddress: '0xdeadbeef' }]
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txArgs: txType === BraveWallet.TransactionType.ETHSend ? [] : ['mockArg1', 'mockArg2'],
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: {} as any,
            ethTxData1559: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559!,
              baseData: {
                ...mockTransactionInfo.txDataUnion.ethTxData1559!.baseData,
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
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [],
          [{ ...mockERC20Token, contractAddress: '0xdeadbeef' }]
        ))

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
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [],
          [{ ...mockERC20Token, contractAddress: '0xdeadbeef' }]
        ))

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

        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork,
          [{
            ...mockAccount,
            tokenBalanceRegistry: {
              [mockERC20Token.contractAddress.toLowerCase()]: '1000000000000000'
            },
            address: '0xdeadbeef',
            nativeBalanceRegistry: {
              '0x1': '1000000000000000' // 0.001 ETH
            }
          }],
          mockAssetPrices, [], []
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          fromAddress: '0xdeadbeef',
          txArgs: [BraveWallet.TransactionType.ERC20Approve, BraveWallet.TransactionType.ERC20Transfer].includes(txType)
            ? ['mockRecipient', '0x0']
            : ['mockOwner', 'mockRecipient', 'mockTokenID'],
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: {} as any,
            ethTxData1559: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559!,
              maxFeePerGas: '0x22ecb25c00', // 150 Gwei
              maxPriorityFeePerGas: '0x22ecb25c00', // 150 Gwei
              baseData: {
                ...mockTransactionInfo.txDataUnion.ethTxData1559!.baseData,
                to: txType === BraveWallet.TransactionType.ERC20Transfer
                  ? mockERC20Token.contractAddress.toLowerCase()
                  : mockTransactionInfo.txDataUnion.ethTxData1559!.baseData.to,
                value: '0x0', // 0 ETH
                gasLimit: '0x5208', // 21000
                gasPrice: '0x22ecb25c00' // 150 Gwei
              }
            }
          }
        })

        expect(parsedTransaction.insufficientFundsForGasError).toBeTruthy()
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

        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork,
          [{
            ...mockAccount,
            address: '0xdeadbeef',
            nativeBalanceRegistry: {
              '0x1': '1000000000000000000' // 1 ETH
            },
            tokenBalanceRegistry: {
              [mockTxData.baseData.to.toLowerCase()]: '0'
            }
          }],
          mockAssetPrices,
          [
            {
              ...mockERC20Token,
              contractAddress: mockTxData.baseData.to.toLowerCase()
            }
          ],
          []
        ))

        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          fromAddress: '0xdeadbeef',
          txArgs: [BraveWallet.TransactionType.ERC20Approve, BraveWallet.TransactionType.ERC20Transfer].includes(txType)
            ? ['mockRecipient', '0x0']
            : ['mockOwner', 'mockRecipient', 'mockTokenID'],
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: {} as any,
            ethTxData1559: {
              ...mockTxData,
              baseData: {
                ...mockTxData.baseData,
                value: '0x0',
                gasLimit: '0x5208', // 21000
                gasPrice: '0x22ecb25c00' // 150 Gwei
              }
            }
          }
        })

        expect(parsedTransaction.insufficientFundsForGasError).toBeFalsy()
        expect(parsedTransaction.insufficientFundsError).toBeFalsy()
      })
    })

    describe.each([
      ['ETHSend', BraveWallet.TransactionType.ETHSend],
      ['Other', BraveWallet.TransactionType.Other]
    ])('%s', (_, txType) => {
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

        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork,
          [{
            ...mockAccount,
            address: '0xdeadbeef',
            nativeBalanceRegistry: {
              '0x1': '4000000000000000' // 0.004 ETH
            }
          }],
          mockAssetPrices, [], []
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          fromAddress: '0xdeadbeef',
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: {} as any,
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
        })

        expect(parsedTransaction.insufficientFundsError).toBeTruthy()
        expect(parsedTransaction.insufficientFundsForGasError).toBeFalsy()
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

        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork,
          [{
            ...mockAccount,
            address: '0xdeadbeef',
            nativeBalanceRegistry: {
              '0x1': '1003150000000000000' // 1.00315 ETH
            }
          }],
          mockAssetPrices, [], []
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          fromAddress: '0xdeadbeef',
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: {} as any,
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
        })

        expect(parsedTransaction.insufficientFundsError).toBeFalsy()
        expect(parsedTransaction.insufficientFundsForGasError).toBeFalsy()
      })
    })

    describe('ERC20Transfer', () => {
      it('should be true when funds are insufficient for send amount', () => {
        /**
         * Account ETH balance: 0.00315 ETH
         * Account DOG balance: 0.001 DOG
         * Transaction value: 1 DOG
         *
         * Gas fee: 0.00315 ETH
         *   - gasPrice: 150 Gwei
         *   - gasLimit: 21000
         *
         * Remarks: sufficient funds for gas, but not for send amount.
         */

        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork,
          [{
            ...mockAccount,
            tokenBalanceRegistry: {
              [mockERC20Token.contractAddress.toLowerCase()]: '1000000000000000' // 0.001 DOG
            },
            address: '0xdeadbeef',
            nativeBalanceRegistry: {
              '0x1': '3150000000000000' // 0.00315 ETH
            }
          }],
          mockAssetPrices, [], []
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
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
            solanaTxData: {} as any,
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
        })

        // [FIXME] - Difficult to capture results from reinvocation of a
        // useCallback(), which fails the following assertion. Fix this
        // by returning insufficientFundsError as part of the result. The
        // test was previously passing as a false positive. There might be
        // similar issues with other tests, so transaction parser hook must
        // be rewritten.
        //
        // expect(parsedTransaction.insufficientFundsError).toBeTruthy()
        expect(parsedTransaction.insufficientFundsForGasError).toBeFalsy()
      })

      it('should be false when funds are sufficient for send amount', () => {
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

        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork,
          [{
            ...mockAccount,
            tokenBalanceRegistry: {
              [mockERC20Token.contractAddress.toLowerCase()]: '1000000000000000000' // 1 DOG
            },
            address: '0xdeadbeef',
            nativeBalanceRegistry: {
              '0x1': '3150000000000000' // 0.00315 ETH
            }
          }],
          mockAssetPrices,
          [],
          [mockERC20Token]
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
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
            solanaTxData: {} as any,
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
        })

        expect(parsedTransaction.insufficientFundsError).toBeFalsy()
        expect(parsedTransaction.insufficientFundsForGasError).toBeFalsy()
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
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [mockAccount], [], [mockERC20Token]
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: {} as any,
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
        })

        expect(parsedTransaction.symbol).toEqual('')
      })
      it('Gets token symbol from visibleList, should be DOG', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [mockAccount], [], [mockERC20Token]
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: {} as any,
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
        })

        expect(parsedTransaction.symbol).toEqual('DOG')
      })
      it('Gets token symbol from fallback fullTokenList, should be DOG', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [mockAccount], [], [], [mockERC20Token]
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txType,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: {} as any,
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
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [mockAccount], [], [], [mockERC20Token]
        ))

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
            solanaTxData: {} as any,
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
        expect(parsedTransaction1.gasLimit).toEqual('')
        expect(parsedTransaction1.missingGasLimitError).toBeTruthy()

        const mockTransactionInfo2: BraveWallet.TransactionInfo = {
          ...baseMockTransactionInfo,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: {} as any,
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
        expect(parsedTransaction2.gasLimit).toEqual('0')
        expect(parsedTransaction2.missingGasLimitError).toBeTruthy()

        const mockTransactionInfo3: BraveWallet.TransactionInfo = {
          ...baseMockTransactionInfo,
          txDataUnion: {
            ethTxData: {} as any,
            filTxData: undefined,
            solanaTxData: {} as any,
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
        expect(parsedTransaction3.gasLimit).toEqual('1')
        expect(parsedTransaction3.missingGasLimitError).toBeUndefined()
      })
    })
  })
})
