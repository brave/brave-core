// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import {
  getMockedTransactionInfo,
  mockAccount,
  mockNetwork,
} from '../common/constants/mocks'
import { SwapExchangeProxy } from '../common/constants/registry'
import { BraveWallet, SerializableTransactionInfo } from '../constants/types'
import { makeNetworkAsset } from '../options/asset-options'
import {
  mockBasicAttentionToken,
  mockBitcoinErc20Token,
  mockErc20TokensList
} from '../stories/mock-data/mock-asset-options'
import { mockEthMainnet } from '../stories/mock-data/mock-networks'
import { mockFilSendTransaction, mockTransactionInfo } from '../stories/mock-data/mock-transaction-info'
import { mockEthAccount } from '../stories/mock-data/mock-wallet-accounts'
import Amount from './amount'
import {
  accountHasInsufficientFundsForGas,
  accountHasInsufficientFundsForTransaction,
  findTransactionToken,
  getETHSwapTransactionBuyAndSellTokens,
  getIsRevokeApprovalTx,
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
        effectiveRecipient: mockAccount.address,
        fromAccountId: mockAccount.accountId,
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

describe('check for insufficient funds errors', () => {
  describe.each([
    ['ERC20Transfer', BraveWallet.TransactionType.ERC20Transfer],
    ['ERC20Approve', BraveWallet.TransactionType.ERC20Approve],
    ['ERC721TransferFrom', BraveWallet.TransactionType.ERC721TransferFrom],
    [
      'ERC721SafeTransferFrom',
      BraveWallet.TransactionType.ERC721SafeTransferFrom
    ],
    ['ETHSend', BraveWallet.TransactionType.ETHSend],
    ['Other', BraveWallet.TransactionType.Other]
  ])('%s', (_, txType) => {
    it('should correctly indicate when funds are insufficient for gas', () => {
      const mockTransactionInfo = getMockedTransactionInfo()

      /**
       * Account balance: 0.001 ETH
       * Transaction value: 0 ETH
       *
       * Gas fee: 0.00315 ETH
       *   - gasPrice: 150 Gwei
       *   - gasLimit: 21000
       */
      const nativeBalanceRegistry = {
        [mockTransactionInfo.chainId]: '1000000000000000' // 0.001 ETH
      }

      const accountNativeBalance =
        nativeBalanceRegistry[mockTransactionInfo.chainId]

      const insufficientFundsForGasError = accountHasInsufficientFundsForGas({
        accountNativeBalance,
        gasFee: '3150000000000000' // 0.00315 ETH
      })

      expect(insufficientFundsForGasError).toBeTruthy()
    })

    it('should be false when funds are sufficient for gas', () => {
      /**
       * Account balance: 1 ETH
       * Transaction value: 0 ETH
       */
      const nativeBalanceRegistry = {
        '0x1': '1000000000000000000' // 1 ETH
      }
      const tokenBalanceRegistry = {
        '0x07865c6e87b9f70255377e024ace6630c1eaa37f': '450346',
        '0xc3f733ca98E0daD0386979Eb96fb1722A1A05E69': '450346'
      }
      /**
       * Gas fee: 0.00315 ETH
       */
      const gasFee = '3150000000000000' // 0.00315 ETH
      const mockTxData: BraveWallet.TxData1559 =
        getMockedTransactionInfo().txDataUnion.ethTxData1559

      const mockTransactionInfo = {
        ...getMockedTransactionInfo(),
        fromAddress: '0xdeadbeef',
        txArgs: [
          BraveWallet.TransactionType.ERC20Approve,
          BraveWallet.TransactionType.ERC20Transfer
        ].includes(txType)
          ? ['mockRecipient', '0x0']
          : ['mockOwner', 'mockRecipient', 'mockTokenID'],
        txType,
        txDataUnion: {
          ethTxData: {} as any,
          filTxData: undefined,
          solanaTxData: undefined,
          btcTxData: undefined,
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
      }

      const insufficientFundsForGasError = accountHasInsufficientFundsForGas({
        accountNativeBalance:
          nativeBalanceRegistry[mockTransactionInfo.chainId],
        gasFee
      })

      const token = findTransactionToken(
        mockTransactionInfo,
        mockErc20TokensList
      )

      const {
        sellAmountWei,
        sellToken
      } = getETHSwapTransactionBuyAndSellTokens({
        tokensList: mockErc20TokensList,
        tx: mockTransactionInfo,
        nativeAsset: {
          ...makeNetworkAsset(mockEthMainnet),
          chainId: mockTransactionInfo.chainId
        }
      })

      const accountNativeBalance =
        nativeBalanceRegistry[mockTransactionInfo.chainId]

      const sellTokenBalance =
        tokenBalanceRegistry[sellToken?.contractAddress ?? '']

      const tokenBalance = tokenBalanceRegistry[token?.contractAddress ?? '']

      const insufficientFundsError = accountHasInsufficientFundsForTransaction({
        accountNativeBalance,
        accountTokenBalance: tokenBalance,
        gasFee,
        sellAmountWei,
        sellTokenBalance: sellTokenBalance,
        tx: mockTransactionInfo
      })

      expect(insufficientFundsForGasError).toBeFalsy()
      expect(insufficientFundsError).toBeFalsy()
    })
  })

  describe.each([
    ['ETHSend', BraveWallet.TransactionType.ETHSend],
    ['Other', BraveWallet.TransactionType.Other]
  ])('%s', (_, txType) => {
    const mockTransactionInfo = getMockedTransactionInfo()
    const transactionInfo: SerializableTransactionInfo = {
      ...mockTransactionInfo,
      fromAddress: '0xdeadbeef',
      txType,
      txDataUnion: {
        ethTxData: {} as any,
        filTxData: undefined,
        solanaTxData: undefined,
        btcTxData: undefined,
        ethTxData1559: {
          ...mockTransactionInfo.txDataUnion.ethTxData1559,
          baseData: {
            ...mockTransactionInfo.txDataUnion.ethTxData1559.baseData,
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
      const gasFee = '3150000000000000' // 0.00315 ETH
      const nativeBalanceRegistry = {
        [transactionInfo.chainId]: '4000000000000000' // 0.004 ETH
      }

      const insufficientFundsForGasError = accountHasInsufficientFundsForGas({
        accountNativeBalance:
          nativeBalanceRegistry[transactionInfo.chainId],
        gasFee
      })

      const insufficientFundsError = accountHasInsufficientFundsForTransaction({
        accountNativeBalance: nativeBalanceRegistry[transactionInfo.chainId],
        accountTokenBalance: '',
        gasFee,
        sellAmountWei: Amount.empty(),
        sellTokenBalance: '',
        tx: transactionInfo
      })


      expect(insufficientFundsError).toBeTruthy()
      expect(insufficientFundsForGasError).toBeFalsy()
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
      const gasFee = '3150000000000000' // 0.00315 ETH
      const nativeBalanceRegistry = {
        '0x1': '1003150000000000000' // 1.00315 ETH
      }
      const accountNativeBalance =
        nativeBalanceRegistry[mockTransactionInfo.chainId]

      const insufficientFundsForGasError = accountHasInsufficientFundsForGas({
        accountNativeBalance,
        gasFee
      })

      const insufficientFundsError = accountHasInsufficientFundsForTransaction({
        accountNativeBalance,
        accountTokenBalance: '',
        gasFee,
        sellAmountWei: Amount.empty(),
        sellTokenBalance: '',
        tx: mockTransactionInfo
      })

      expect(insufficientFundsError).toBeFalsy()
      expect(insufficientFundsForGasError).toBeFalsy()
    })
  })

  describe('ERC20Transfer', () => {
    it('should be true when funds are insufficient for send amount', () => {
      /**
       * Account ETH balance: 0.00315 ETH
       * Account Token balance: 0.99
       * Transaction value: 1 token
       *
       * Gas fee: 0.00315 ETH
       *   - gasPrice: 150 Gwei
       *   - gasLimit: 21000
       *
       * Remarks: sufficient funds for gas, but not for send amount.
       */
      const mockTransactionInfo = getMockedTransactionInfo()
      const transferredToken = mockErc20TokensList[0]
      const sendAmount = new Amount('1')
        .multiplyByDecimals(transferredToken.decimals)
        .toHex() // 1 full Token
      const transactionInfo: SerializableTransactionInfo = {
        ...mockTransactionInfo,
        fromAddress: '0xdeadbeef',
        // (address recipient, uint256 amount)
        txArgs: ['mockRecipient', sendAmount],
        txType: BraveWallet.TransactionType.ERC20Transfer,
        txDataUnion: {
          ethTxData: {} as any,
          filTxData: undefined,
          solanaTxData: undefined,
          btcTxData: undefined,
          ethTxData1559: {
            ...mockTransactionInfo.txDataUnion.ethTxData1559,
            baseData: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559.baseData,
              to: transferredToken.contractAddress,
              value: '0x0', // 0 ETH
              gasLimit: '0x5208', // 21000
              gasPrice: '0x22ecb25c00' // 150 Gwei
            }
          }
        }
      }
      const gasFee = '3150000000000000' // 0.00315 ETH
      const nativeBalanceRegistry = {
        [transactionInfo.chainId]: '1003150000000000000' // 1.00315 ETH
      }
      const tokenBalanceRegistry = {
        [transferredToken.contractAddress]: new Amount('0.99')
          .multiplyByDecimals(transferredToken.decimals)
          .toHex() // less than 1 full Token
      }

      const insufficientFundsForGasError = accountHasInsufficientFundsForGas({
        accountNativeBalance: nativeBalanceRegistry[transactionInfo.chainId],
        gasFee
      })

      const token = findTransactionToken(transactionInfo, mockErc20TokensList)

      const { sellAmountWei, sellToken } =
        getETHSwapTransactionBuyAndSellTokens({
          tokensList: mockErc20TokensList,
          tx: transactionInfo,
          nativeAsset: {
            ...makeNetworkAsset(mockEthMainnet),
            chainId: transactionInfo.chainId
          }
        })

      const insufficientFundsError = accountHasInsufficientFundsForTransaction({
        accountNativeBalance: nativeBalanceRegistry[transactionInfo.chainId],
        accountTokenBalance: tokenBalanceRegistry[token?.contractAddress ?? ''],
        gasFee,
        sellAmountWei,
        sellTokenBalance:
          tokenBalanceRegistry[sellToken?.contractAddress ?? ''],
        tx: transactionInfo
      })

      expect(nativeBalanceRegistry[transactionInfo.chainId]).toBeTruthy()
      expect(token).toBeTruthy()
      expect(tokenBalanceRegistry[token?.contractAddress ?? '']).toBeTruthy()
      expect(insufficientFundsError).toBeTruthy()
      expect(insufficientFundsForGasError).toBeFalsy()
    })

    it('should be false when funds are sufficient for send amount', () => {
      /**
       * Account ETH balance: 0.00315 ETH
       * Account Token balance: 2
       * Transaction value: 0.5 tokens
       *
       * Gas fee: 0.00315 ETH
       *   - gasPrice: 150 Gwei
       *   - gasLimit: 21000
       *
       * Remarks: sufficient funds for gas, but not for send amount.
       */
      const mockTransactionInfo = getMockedTransactionInfo()
      const transferredToken = mockErc20TokensList[0]
      const sendAmount = new Amount('0.5')
        .multiplyByDecimals(transferredToken.decimals)
        .toHex() // half of a full token

      const transactionInfo: SerializableTransactionInfo = {
        ...mockTransactionInfo,
        fromAddress: '0xdeadbeef',
        // (address recipient, uint256 amount)
        txArgs: ['mockRecipient', sendAmount],
        txType: BraveWallet.TransactionType.ERC20Transfer,
        txDataUnion: {
          ethTxData: {} as any,
          filTxData: undefined,
          solanaTxData: undefined,
          btcTxData: undefined,
          ethTxData1559: {
            ...mockTransactionInfo.txDataUnion.ethTxData1559,
            baseData: {
              ...mockTransactionInfo.txDataUnion.ethTxData1559.baseData,
              to: transferredToken.contractAddress,
              value: '0x0', // 0 ETH
              gasLimit: '0x5208', // 21000
              gasPrice: '0x22ecb25c00' // 150 Gwei
            }
          }
        }
      }
      const gasFee = '3150000000000000' // 0.00315 ETH
      const nativeBalanceRegistry = {
        [transactionInfo.chainId]: '1003150000000000000' // 1.00315 ETH
      }
      const tokenBalanceRegistry = {
        [transferredToken.contractAddress]: new Amount('2')
          .multiplyByDecimals(transferredToken.decimals)
          .toHex() // 2 full Tokens
      }

      const insufficientFundsForGasError = accountHasInsufficientFundsForGas({
        accountNativeBalance: nativeBalanceRegistry[transactionInfo.chainId],
        gasFee
      })

      const token = findTransactionToken(transactionInfo, mockErc20TokensList)

      const { sellAmountWei, sellToken } =
        getETHSwapTransactionBuyAndSellTokens({
          tokensList: mockErc20TokensList,
          tx: transactionInfo,
          nativeAsset: {
            ...makeNetworkAsset(mockEthMainnet),
            chainId: transactionInfo.chainId
          }
        })

      const insufficientFundsError = accountHasInsufficientFundsForTransaction({
        accountNativeBalance: nativeBalanceRegistry[transactionInfo.chainId],
        accountTokenBalance: tokenBalanceRegistry[token?.contractAddress ?? ''],
        gasFee,
        sellAmountWei,
        sellTokenBalance:
          tokenBalanceRegistry[sellToken?.contractAddress ?? ''],
        tx: transactionInfo
      })

      expect(insufficientFundsError).toBeFalsy()
      expect(insufficientFundsForGasError).toBeFalsy()
    })
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
      value: 'value'
    }

    const union = toTxDataUnion({ filTxData: filTxData })

    expect(Object.keys(union).length).toBe(1)
    expect(union.filTxData).toBe(filTxData)
    expect(union.ethTxData).toBe(undefined)
    expect(union.ethTxData1559).toBe(undefined)
    expect(union.solanaTxData).toBe(undefined)
    expect(union.btcTxData).toBe(undefined)
  })
})

describe('getIsRevokeApprovalTx', () => {
  test('correctly detects revocations', () => {
    expect(getIsRevokeApprovalTx({
      ...mockTransactionInfo,
      txType: BraveWallet.TransactionType.ERC20Approve,
      txArgs: [
        mockEthAccount.address, // spender
        '0' // amount
      ]
    })).toBe(true)
  })
  test('correctly detects non-revocation approvals', () => {
    expect(getIsRevokeApprovalTx({
      ...mockTransactionInfo,
      txType: BraveWallet.TransactionType.ERC20Approve,
      txArgs: [
        mockEthAccount.address, // spender
        '10' // amount
      ]
    })).toBe(false)
  })
})
