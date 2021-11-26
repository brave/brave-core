import { renderHook } from '@testing-library/react-hooks'

import { TransactionType } from '../../constants/types'
import {
  getMockedTransactionInfo,
  mockERC20Token,
  mockNetwork
} from '../constants/mocks'
import { SwapExchangeProxy } from './address-labels'
import { useTransactionParser } from './transaction-parser'

describe('useTransactionParser hook', () => {
  describe('check for sameAddressError field', () => {
    describe.each([
      ['ERC20Transfer', TransactionType.ERC20Transfer, 'recipient'],
      ['ERC20Approve', TransactionType.ERC20Approve, 'approval target']
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
      ['ERC721TransferFrom', TransactionType.ERC721TransferFrom],
      ['ERC721SafeTransferFrom', TransactionType.ERC721SafeTransferFrom]
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
      ['ETHSend', TransactionType.ETHSend],
      ['Other', TransactionType.Other],
      ['0x Swap', TransactionType.Other]
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
          txData: {
            ...mockTransactionInfo.txData,
            baseData: {
              ...mockTransactionInfo.txData.baseData,
              to: name === '0x Swap' ? SwapExchangeProxy : '0xdeadbeef'
            }
          }
        })

        expect(parsedTransaction.sameAddressError).toBeUndefined()
      })
    })
  })

  describe('check for contractAddressError', () => {
    describe.each([
      ['ERC20Approve', TransactionType.ERC20Approve],
      ['ETHSend', TransactionType.ETHSend],
      ['Other', TransactionType.Other],
      ['0x Swap', TransactionType.Other]
    ])('%s', (name, txType) => {
      it('should always be undefined', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [],
          [{ ...mockERC20Token, contractAddress: '0xdeadbeef' }]
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txArgs: txType === TransactionType.ETHSend ? [] : ['mockArg1', 'mockArg2'],
          txType,
          txData: {
            ...mockTransactionInfo.txData,
            baseData: {
              ...mockTransactionInfo.txData.baseData,
              to: name === '0x Swap' ? SwapExchangeProxy : '0xdeadbeef'
            }
          }
        })

        expect(parsedTransaction.contractAddressError).toBeUndefined()
      })
    })

    describe.each([
      ['ERC20Transfer', TransactionType.ERC20Transfer],
      ['ERC721TransferFrom', TransactionType.ERC721TransferFrom],
      ['ERC721SafeTransferFrom', TransactionType.ERC721SafeTransferFrom]
    ])('%s', (_, txType) => {
      it('should be defined when recipient is a known contract address', () => {
        const { result: { current: transactionParser } } = renderHook(() => useTransactionParser(
          mockNetwork, [], [], [],
          [{ ...mockERC20Token, contractAddress: '0xdeadbeef' }]
        ))

        const mockTransactionInfo = getMockedTransactionInfo()
        const parsedTransaction = transactionParser({
          ...mockTransactionInfo,
          txArgs: txType === TransactionType.ERC20Transfer
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
          txArgs: txType === TransactionType.ERC20Transfer
            ? ['0xbadcafe', 'mockAmount']
            : ['mockOwner', '0xbadcafe', 'mockTokenID'],
          txType
        })

        expect(parsedTransaction.contractAddressError).toBeUndefined()
      })
    })
  })
})
