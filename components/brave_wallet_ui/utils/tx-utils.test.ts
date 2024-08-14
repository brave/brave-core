// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { getLocale } from '../../common/locale'
import {
  getMockedTransactionInfo,
  mockAccount,
  mockNetwork,
  mockSolanaAccount
} from '../common/constants/mocks'
import { SwapExchangeProxy } from '../common/constants/registry'
import { BraveWallet, SerializableTransactionInfo } from '../constants/types'
import { makeNetworkAsset } from '../options/asset-options'
import {
  mockBasicAttentionToken,
  mockBitcoinErc20Token,
  mockBtcToken,
  mockErc20TokensList,
  mockEthToken,
  mockFilToken,
  mockMoonCatNFT,
  mockSolToken,
  mockSplBat,
  mockZecToken
} from '../stories/mock-data/mock-asset-options'
import { mockEthMainnet } from '../stories/mock-data/mock-networks'
import {
  createMockTransactionInfo,
  mockBtcSendTransaction,
  mockEthSendTransaction,
  mockFilSendTransaction,
  mockSolanaTransactionInfo,
  mockTransactionInfo,
  mockZecSendTransaction
} from '../stories/mock-data/mock-transaction-info'
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
  getTransactionTypeName,
  toTxDataUnion
} from './tx-utils'

describe('Check Transaction Status Strings Value', () => {
  test('Transaction ID 0 should return Unapproved', () => {
    expect(getTransactionStatusString(0)).toEqual(
      'braveWalletTransactionStatusUnapproved'
    )
  })
  test('Transaction ID 1 should return Approved', () => {
    expect(getTransactionStatusString(1)).toEqual(
      'braveWalletTransactionStatusApproved'
    )
  })

  test('Transaction ID 2 should return Rejected', () => {
    expect(getTransactionStatusString(2)).toEqual(
      'braveWalletTransactionStatusRejected'
    )
  })

  test('Transaction ID 3 should return Submitted', () => {
    expect(getTransactionStatusString(3)).toEqual(
      'braveWalletTransactionStatusSubmitted'
    )
  })

  test('Transaction ID 4 should return Confirmed', () => {
    expect(getTransactionStatusString(4)).toEqual(
      'braveWalletTransactionStatusConfirmed'
    )
  })

  test('Transaction ID 5 should return Error', () => {
    expect(getTransactionStatusString(5)).toEqual(
      'braveWalletTransactionStatusError'
    )
  })

  test('Transaction ID 6 should return Dropped', () => {
    expect(getTransactionStatusString(6)).toEqual(
      'braveWalletTransactionStatusDropped'
    )
  })

  test('Transaction ID 7 should return Signed', () => {
    expect(getTransactionStatusString(7)).toEqual(
      'braveWalletTransactionStatusSigned'
    )
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
    const { buyAmountWei, sellAmountWei, buyToken, sellToken } =
      getETHSwapTransactionBuyAndSellTokens({
        tokensList: mockErc20TokensList,
        tx: {
          chainId: BraveWallet.MAINNET_CHAIN_ID,
          confirmedTime: { microseconds: Date.now() },
          createdTime: { microseconds: Date.now() },
          fromAddress: mockAccount.address,
          effectiveRecipient: mockAccount.address,
          fromAccountId: mockAccount.accountId,
          id: 'swap',
          originInfo: undefined,
          submittedTime: { microseconds: Date.now() },
          txArgs: [],
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
          txType: BraveWallet.TransactionType.ETHSwap,
          isRetriable: false,
          swapInfo: {
            fromCoin: mockBasicAttentionToken.coin,
            fromChainId: mockBasicAttentionToken.chainId,
            fromAsset: mockBasicAttentionToken.contractAddress,
            fromAmount: '1',
            toCoin: mockBitcoinErc20Token.coin,
            toChainId: mockBitcoinErc20Token.chainId,
            toAsset: mockBitcoinErc20Token.contractAddress,
            toAmount: '2'
          } as BraveWallet.SwapInfo
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
    expect(buyAmountWei.format()).toEqual('2')
    expect(sellAmountWei.format()).toEqual('1')
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

      const nativeAsset = {
        ...makeNetworkAsset(mockEthMainnet),
        chainId: mockTransactionInfo.chainId
      }

      const token = findTransactionToken(
        mockTransactionInfo,
        mockErc20TokensList
      )

      const { sellAmountWei, sellToken } =
        getETHSwapTransactionBuyAndSellTokens({
          tokensList: mockErc20TokensList,
          tx: mockTransactionInfo,
          nativeAsset
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
        zecTxData: undefined,
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
        accountNativeBalance: nativeBalanceRegistry[transactionInfo.chainId],
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
          zecTxData: undefined,
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

      const nativeAsset = {
        ...makeNetworkAsset(mockEthMainnet),
        chainId: transactionInfo.chainId
      }

      const token = findTransactionToken(transactionInfo, mockErc20TokensList)

      const { sellAmountWei, sellToken } =
        getETHSwapTransactionBuyAndSellTokens({
          tokensList: mockErc20TokensList,
          tx: transactionInfo,
          nativeAsset
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
          zecTxData: undefined,
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

      const nativeAsset = {
        ...makeNetworkAsset(mockEthMainnet),
        chainId: transactionInfo.chainId
      }

      const token = findTransactionToken(transactionInfo, mockErc20TokensList)

      const { sellAmountWei, sellToken } =
        getETHSwapTransactionBuyAndSellTokens({
          tokensList: mockErc20TokensList,
          tx: transactionInfo,
          nativeAsset
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
    expect(union.zecTxData).toBe(undefined)
  })
})

describe('getIsRevokeApprovalTx', () => {
  test('correctly detects revocations', () => {
    expect(
      getIsRevokeApprovalTx({
        ...mockTransactionInfo,
        txType: BraveWallet.TransactionType.ERC20Approve,
        txArgs: [
          mockEthAccount.address, // spender
          '0' // amount
        ]
      })
    ).toBe(true)
  })
  test('correctly detects non-revocation approvals', () => {
    expect(
      getIsRevokeApprovalTx({
        ...mockTransactionInfo,
        txType: BraveWallet.TransactionType.ERC20Approve,
        txArgs: [
          mockEthAccount.address, // spender
          '10' // amount
        ]
      })
    ).toBe(false)
  })
})

describe('findTransactionToken', () => {
  describe('Native asset Transfers', () => {
    it(
      'should detect SOL ' +
        'as the token for Solana System transfer transactions',
      () => {
        expect(
          findTransactionToken(mockSolanaTransactionInfo, [mockSolToken])
            ?.symbol
        ).toBe('SOL')
      }
    )

    it('should detect FIL as the token for Filecoin send transactions', () => {
      expect(
        findTransactionToken(mockFilSendTransaction, [mockFilToken])?.symbol
      ).toBe('FIL')
    })

    it('should detect ETH as the token for Ethereum send transactions', () => {
      expect(
        findTransactionToken(mockEthSendTransaction, [mockEthToken])?.symbol
      ).toBe('ETH')
    })

    it('should detect BTC as the token for Bitcoin transactions', () => {
      expect(
        findTransactionToken(mockBtcSendTransaction, [mockBtcToken])?.symbol
      ).toBe('BTC')
    })

    it('should detect ZEC as the token for ZCash transactions', () => {
      expect(
        findTransactionToken(mockZecSendTransaction, [mockZecToken])?.symbol
      ).toBe('ZEC')
    })
  })

  describe('Token Transfers', () => {
    it('should detect sent ERC20 tokens', () => {
      expect(
        findTransactionToken(
          createMockTransactionInfo({
            chainId: BraveWallet.MAINNET_CHAIN_ID,
            coinType: BraveWallet.CoinType.ETH,
            fromAccount: mockEthAccount,
            sendApproveOrSellAmount: '1000',
            sendApproveOrSellAssetContractAddress:
              mockBasicAttentionToken.contractAddress,
            toAddress: mockAccount.address,
            isERC20Send: true
          }),
          [mockEthToken, mockBasicAttentionToken]
        )?.symbol
      ).toBe('BAT')
    })

    it('should detect sent ERC721 tokens', () => {
      expect(
        findTransactionToken(
          createMockTransactionInfo({
            chainId: BraveWallet.MAINNET_CHAIN_ID,
            coinType: BraveWallet.CoinType.ETH,
            fromAccount: mockEthAccount,
            sendApproveOrSellAmount: '1000',
            sendApproveOrSellAssetContractAddress:
              mockMoonCatNFT.contractAddress,
            toAddress: mockAccount.address,
            isERC721Send: true
          }),
          [mockEthToken, mockMoonCatNFT]
        )?.symbol
      ).toBe('AMC')
    })

    it('should detect sent SPL tokens', () => {
      expect(
        findTransactionToken(
          createMockTransactionInfo({
            chainId: BraveWallet.SOLANA_MAINNET,
            coinType: BraveWallet.CoinType.SOL,
            fromAccount: mockSolanaAccount,
            sendApproveOrSellAmount: '1000',
            sendApproveOrSellAssetContractAddress: mockSplBat.contractAddress,
            toAddress: mockSolanaAccount.address
          }),
          [mockSolToken, mockSplBat]
        )?.symbol
      ).toBe('BAT')
    })
  })
})

describe('getTransactionTypeName', () => {
  test.each([
    {
      txType: BraveWallet.TransactionType.ERC1155SafeTransferFrom,
      expectedString: getLocale(
        'braveWalletTransactionTypeNameSafeTransferFrom'
      )
    },

    {
      txType: BraveWallet.TransactionType.ERC20Approve,
      expectedString: getLocale('braveWalletTransactionTypeNameErc20Approve')
    },

    {
      txType: BraveWallet.TransactionType.ERC20Transfer,
      expectedString: getLocale('braveWalletTransactionTypeNameTokenTransfer')
    },

    {
      txType: BraveWallet.TransactionType.ERC721SafeTransferFrom,
      expectedString: getLocale(
        'braveWalletTransactionTypeNameSafeTransferFrom'
      )
    },

    {
      txType: BraveWallet.TransactionType.ERC721TransferFrom,
      expectedString: getLocale('braveWalletTransactionTypeNameNftTransfer')
    },

    {
      txType: BraveWallet.TransactionType.ETHFilForwarderTransfer,
      expectedString: getLocale('braveWalletTransactionTypeNameForwardFil')
    },

    {
      txType: BraveWallet.TransactionType.ETHSend,
      expectedString: getLocale('braveWalletTransactionIntentSend').replace(
        '$1',
        'ETH'
      )
    },

    {
      txType: BraveWallet.TransactionType.ETHSwap,
      expectedString: getLocale('braveWalletSwap')
    },

    {
      txType: BraveWallet.TransactionType.Other,
      expectedString: getLocale('braveWalletTransactionTypeNameOther')
    },

    {
      txType: BraveWallet.TransactionType.SolanaCompressedNftTransfer,
      expectedString: getLocale(
        'braveWalletTransactionTypeNameCompressedNftTransfer'
      )
    },

    {
      txType: BraveWallet.TransactionType.SolanaDappSignAndSendTransaction,
      expectedString: getLocale(
        'braveWalletTransactionTypeNameSignAndSendDappTransaction'
      )
    },

    {
      txType: BraveWallet.TransactionType.SolanaDappSignTransaction,
      expectedString: getLocale(
        'braveWalletTransactionTypeNameSignDappTransaction'
      )
    },

    {
      txType: BraveWallet.TransactionType.SolanaSPLTokenTransfer,
      expectedString: getLocale('braveWalletTransactionTypeNameTokenTransfer')
    },

    {
      txType:
        BraveWallet.TransactionType
          .SolanaSPLTokenTransferWithAssociatedTokenAccountCreation,
      expectedString: getLocale(
        'braveWalletTransactionTypeNameSplTokenTransferWithAssociatedTokenAccountCreation'
      )
    },

    {
      txType: BraveWallet.TransactionType.SolanaSwap,
      expectedString: getLocale('braveWalletSwap')
    },

    {
      txType: BraveWallet.TransactionType.SolanaSystemTransfer,
      expectedString: getLocale('braveWalletTransactionIntentSend').replace(
        '$1',
        'SOL'
      )
    }
  ])(
    'renders the correct localized function name per tx type',
    ({ expectedString, txType }) => {
      expect(getTransactionTypeName(txType)).toBe(expectedString)
    }
  )

  test('should return "Other" for unknown tx type', () => {
    expect(getTransactionTypeName(999)).toBe(
      getLocale('braveWalletTransactionTypeNameOther')
    )
  })
})
