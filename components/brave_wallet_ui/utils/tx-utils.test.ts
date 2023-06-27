// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { mockAccount, mockNetwork } from '../common/constants/mocks'
import { SwapExchangeProxy } from '../common/constants/registry'
import { BraveWallet } from '../constants/types'
import { makeNetworkAsset } from '../options/asset-options'
import {
  mockBasicAttentionToken,
  mockBitcoinErc20Token,
  mockErc20TokensList
} from '../stories/mock-data/mock-asset-options'
import { mockFilSendTransaction, mockTransactionInfo } from '../stories/mock-data/mock-transaction-info'
import Amount from './amount'
import {
  getETHSwapTransactionBuyAndSellTokens,
  getTransactionGas,
  getTransactionStatusString,
  toTxDataUnion
} from './tx-utils'

describe('Check Transaction Status Strings Value', () => {
  test('Transaction ID 0 should return Unapproved', () => {
    expect(getTransactionStatusString(0)).toEqual('braveWalletTransactionStatusUnapproved')
  })
  test('Transaction ID 1 should return Approved', () => {
    expect(getTransactionStatusString(1)).toEqual('braveWalletTransactionStatusApproved')
  })

  test('Transaction ID 2 should return Rejected', () => {
    expect(getTransactionStatusString(2)).toEqual('braveWalletTransactionStatusRejected')
  })

  test('Transaction ID 3 should return Submitted', () => {
    expect(getTransactionStatusString(3)).toEqual('braveWalletTransactionStatusSubmitted')
  })

  test('Transaction ID 4 should return Confirmed', () => {
    expect(getTransactionStatusString(4)).toEqual('braveWalletTransactionStatusConfirmed')
  })

  test('Transaction ID 5 should return Error', () => {
    expect(getTransactionStatusString(5)).toEqual('braveWalletTransactionStatusError')
  })

  test('Transaction ID 6 should return Dropped', () => {
    expect(getTransactionStatusString(6)).toEqual('braveWalletTransactionStatusDropped')
  })

  test('Transaction ID 7 should return Signed', () => {
    expect(getTransactionStatusString(7))
      .toEqual('braveWalletTransactionStatusSigned')
  })

  test('Transaction ID 8 should return an empty string', () => {
    expect(getTransactionStatusString(8)).toEqual('')
  })
})

describe('getTransactionGas()', () => {
  it('should get the gas values of a FIL transaction', () => {
    const txGas = getTransactionGas(mockFilSendTransaction)

    const { filTxData } = mockFilSendTransaction.txDataUnion

    expect(txGas.maxFeePerGas).toBe(filTxData.gasFeeCap || '')
    expect(txGas.maxPriorityFeePerGas).toBe(filTxData.gasPremium)
    expect(txGas.gasPrice).toBe(
      new Amount(filTxData.gasFeeCap)
        .minus(filTxData.gasPremium)
        .value?.toString() || ''
    )
  })
  it('should get the gas values of an EVM transaction', () => {
    const txGas = getTransactionGas(mockTransactionInfo)

    const { ethTxData1559 } = mockTransactionInfo.txDataUnion

    expect(txGas.maxFeePerGas).toBe(ethTxData1559?.maxFeePerGas || '')
    expect(txGas.maxPriorityFeePerGas).toBe(
      ethTxData1559?.maxPriorityFeePerGas || ''
    )
    expect(txGas.gasPrice).toBe(ethTxData1559?.baseData.gasPrice || '')
  })
})

describe('getETHSwapTransactionBuyAndSellTokens', () => {
  it('should detect the correct but/swap tokens of a transaction', () => {

    const fillPath = `${
      mockBasicAttentionToken.contractAddress //
    }${
      // only the first token has the "0x" prefix
      mockBitcoinErc20Token.contractAddress.replace('0x', '') //
    }`

    const sellAmountArg = '1'
    const minBuyAmountArg = '2'

    const {
      buyAmountWei,
      sellAmountWei,
      buyToken,
      sellToken
    } = getETHSwapTransactionBuyAndSellTokens({
      tokensList: mockErc20TokensList,
      tx: {
        chainId: BraveWallet.MAINNET_CHAIN_ID,
        confirmedTime: { microseconds: Date.now() },
        createdTime: { microseconds: Date.now() },
        fromAddress: mockAccount.address,
        groupId: undefined,
        id: 'swap',
        originInfo: undefined,
        submittedTime: { microseconds: Date.now() },
        // (bytes fillPath, uint256 sellAmount, uint256 minBuyAmount)
        txArgs: [fillPath, sellAmountArg, minBuyAmountArg],
        txDataUnion: {
          ethTxData1559: {
            baseData: {
              data: [],
              gasLimit: '',
              gasPrice: '',
              nonce: '',
              signedTransaction: '',
              signOnly: false,
              to: SwapExchangeProxy,
              value: ''
            },
            chainId: BraveWallet.MAINNET_CHAIN_ID,
            gasEstimation: undefined,
            maxFeePerGas: '1',
            maxPriorityFeePerGas: '1'
          }
        },
        txHash: '123',
        txParams: [],
        txStatus: BraveWallet.TransactionStatus.Unapproved,
        txType: BraveWallet.TransactionType.ETHSwap
      },
      nativeAsset: makeNetworkAsset(mockNetwork)
    })

    expect(buyToken).toBeDefined()
    expect(sellToken).toBeDefined()
    expect(sellToken?.contractAddress).toBe(
      mockBasicAttentionToken.contractAddress
    )
    expect(buyToken?.contractAddress).toBe(
      mockBitcoinErc20Token.contractAddress
    )
    expect(buyAmountWei.value?.toString()).toEqual(minBuyAmountArg)
    expect(sellAmountWei.value?.toString()).toEqual(sellAmountArg)
  })
})

describe('toTxDataUnion', () => {
  test('works', () => {
    const filTxData: BraveWallet.FilTxData = {
      nonce: '',
      gasPremium: '',
      gasFeeCap: '',
      gasLimit: '',
      maxFee: '0',
      to: 'to',
      from: 'from',
      value: 'value'
    }

    const union = toTxDataUnion({ filTxData: filTxData })

    expect(Object.keys(union).length).toBe(1)
    expect(union.filTxData).toBe(filTxData)
    expect(union.ethTxData).toBe(undefined)
    expect(union.ethTxData1559).toBe(undefined)
    expect(union.solanaTxData).toBe(undefined)
  })
})
